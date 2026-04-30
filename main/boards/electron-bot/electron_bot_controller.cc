/*
    Electron Bot机器人控制器 - MCP协议Version
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "movements.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "ElectronBotController"

struct ElectronBotActionParams {
    int action_type;
    int steps;
    int speed;
    int direction;
    int amount;
};

class ElectronBotController {
private:
    Otto electron_bot_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool is_action_in_progress_ = false;

    enum ActionType {
        // 手部Action 1-12
        ACTION_HAND_LEFT_UP = 1,      // 举左手
        ACTION_HAND_RIGHT_UP = 2,     // 举右手
        ACTION_HAND_BOTH_UP = 3,      // 举双手
        ACTION_HAND_LEFT_DOWN = 4,    // 放左手
        ACTION_HAND_RIGHT_DOWN = 5,   // 放右手
        ACTION_HAND_BOTH_DOWN = 6,    // 放双手
        ACTION_HAND_LEFT_WAVE = 7,    // 挥左手
        ACTION_HAND_RIGHT_WAVE = 8,   // 挥右手
        ACTION_HAND_BOTH_WAVE = 9,    // 挥双手
        ACTION_HAND_LEFT_FLAP = 10,   // 拍打左手
        ACTION_HAND_RIGHT_FLAP = 11,  // 拍打右手
        ACTION_HAND_BOTH_FLAP = 12,   // 拍打双手

        // 身体Action 13-14
        ACTION_BODY_TURN_LEFT = 13,    // 左转
        ACTION_BODY_TURN_RIGHT = 14,   // 右转
        ACTION_BODY_TURN_CENTER = 15,  // 回中心

        // 头部Action 16-20
        ACTION_HEAD_UP = 16,          // 抬头
        ACTION_HEAD_DOWN = 17,        // 低头
        ACTION_HEAD_NOD_ONCE = 18,    // 点头一次
        ACTION_HEAD_CENTER = 19,      // 回中心
        ACTION_HEAD_NOD_REPEAT = 20,  // 连续点头

        // 系统Action 21
        ACTION_HOME = 21  // Reset到初始Position
    };

    static void ActionTask(void* arg) {
        ElectronBotController* controller = static_cast<ElectronBotController*>(arg);
        ElectronBotActionParams params;
        controller->electron_bot_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "执行Action: %d", params.action_type);
                controller->is_action_in_progress_ = true;  // 开始执行Action

                // 执行相应的Action
                if (params.action_type >= ACTION_HAND_LEFT_UP &&
                    params.action_type <= ACTION_HAND_BOTH_FLAP) {
                    // 手部Action
                    controller->electron_bot_.HandAction(params.action_type, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_BODY_TURN_LEFT &&
                           params.action_type <= ACTION_BODY_TURN_CENTER) {
                    // 身体Action
                    int body_direction = params.action_type - ACTION_BODY_TURN_LEFT + 1;
                    controller->electron_bot_.BodyAction(body_direction, params.steps,
                                                         params.amount, params.speed);
                } else if (params.action_type >= ACTION_HEAD_UP &&
                           params.action_type <= ACTION_HEAD_NOD_REPEAT) {
                    // 头部Action
                    int head_action = params.action_type - ACTION_HEAD_UP + 1;
                    controller->electron_bot_.HeadAction(head_action, params.steps, params.amount,
                                                         params.speed);
                } else if (params.action_type == ACTION_HOME) {
                    // ResetAction
                    controller->electron_bot_.Home(true);
                }
                controller->is_action_in_progress_ = false;  // Action执行完毕
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction, int amount) {
        ESP_LOGI(TAG, "Action控制: Class型=%d, 步数=%d, Speed=%d, Direction=%d, 幅度=%d", action_type, steps,
                 speed, direction, amount);

        ElectronBotActionParams params = {action_type, steps, speed, direction, amount};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "electron_bot_action", 1024 * 4, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void LoadTrimsFromNVS() {
        Settings settings("electron_trims", false);

        int right_pitch = settings.GetInt("right_pitch", 0);
        int right_roll = settings.GetInt("right_roll", 0);
        int left_pitch = settings.GetInt("left_pitch", 0);
        int left_roll = settings.GetInt("left_roll", 0);
        int body = settings.GetInt("body", 0);
        int head = settings.GetInt("head", 0);
        electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);
    }

public:
    ElectronBotController() {
        electron_bot_.Init(Right_Pitch_Pin, Right_Roll_Pin, Left_Pitch_Pin, Left_Roll_Pin, Body_Pin,
                           Head_Pin);

        LoadTrimsFromNVS();
        action_queue_ = xQueueCreate(10, sizeof(ElectronBotActionParams));

        QueueAction(ACTION_HOME, 1, 1000, 0, 0);

        RegisterMcpTools();
        ESP_LOGI(TAG, "Electron Bot控制器AlreadyInitialize并RegisterMCP工具");
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "开始RegisterElectron Bot MCP工具...");

        // 手部Action统一工具
        mcp_server.AddTool(
            "self.electron.hand_action",
            "手部Action控制。action: 1=举手, 2=放手, 3=挥手, 4=拍打; hand: 1=左手, 2=右手, 3=双手; "
            "steps: Action重复次数(1-10); speed: ActionSpeed(500-1500，数值越小越快); amount: "
            "Action幅度(10-50，仅举手Action使用)",
            PropertyList({Property("action", kPropertyTypeInteger, 1, 1, 4),
                          Property("hand", kPropertyTypeInteger, 3, 1, 3),
                          Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("amount", kPropertyTypeInteger, 30, 10, 50)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int action_type = properties["action"].value<int>();
                int hand_type = properties["hand"].value<int>();
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int amount = properties["amount"].value<int>();

                // 根据ActionClass型和手部Class型计算具体Action
                int base_action;
                switch (action_type) {
                    case 1:
                        base_action = ACTION_HAND_LEFT_UP;
                        break;  // 举手
                    case 2:
                        base_action = ACTION_HAND_LEFT_DOWN;
                        amount = 0;
                        break;  // 放手
                    case 3:
                        base_action = ACTION_HAND_LEFT_WAVE;
                        amount = 0;
                        break;  // 挥手
                    case 4:
                        base_action = ACTION_HAND_LEFT_FLAP;
                        amount = 0;
                        break;  // 拍打
                    default:
                        base_action = ACTION_HAND_LEFT_UP;
                }
                int action_id = base_action + (hand_type - 1);

                QueueAction(action_id, steps, speed, 0, amount);
                return true;
            });

        // 身体Action
        mcp_server.AddTool(
            "self.electron.body_turn",
            "身体转向。steps: 转向步数(1-10); speed: 转向Speed(500-1500，数值越小越快); direction: "
            "转向Direction(1=左转, 2=右转, 3=回中心); angle: 转向Angle(0-90度)",
            PropertyList({Property("steps", kPropertyTypeInteger, 1, 1, 10),
                          Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                          Property("direction", kPropertyTypeInteger, 1, 1, 3),
                          Property("angle", kPropertyTypeInteger, 45, 0, 90)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int direction = properties["direction"].value<int>();
                int amount = properties["angle"].value<int>();

                int action;
                switch (direction) {
                    case 1:
                        action = ACTION_BODY_TURN_LEFT;
                        break;
                    case 2:
                        action = ACTION_BODY_TURN_RIGHT;
                        break;
                    case 3:
                        action = ACTION_BODY_TURN_CENTER;
                        break;
                    default:
                        action = ACTION_BODY_TURN_LEFT;
                }

                QueueAction(action, steps, speed, 0, amount);
                return true;
            });

        // 头部Action
        mcp_server.AddTool("self.electron.head_move",
                           "头部运动。action: 1=抬头, 2=低头, 3=点头, 4=回中心, 5=连续点头; steps: "
                           "Action重复次数(1-10); speed: ActionSpeed(500-1500，数值越小越快); angle: "
                           "头部转动Angle(1-15度)",
                           PropertyList({Property("action", kPropertyTypeInteger, 3, 1, 5),
                                         Property("steps", kPropertyTypeInteger, 1, 1, 10),
                                         Property("speed", kPropertyTypeInteger, 1000, 500, 1500),
                                         Property("angle", kPropertyTypeInteger, 5, 1, 15)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int action_num = properties["action"].value<int>();
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int amount = properties["angle"].value<int>();
                               int action = ACTION_HEAD_UP + (action_num - 1);
                               QueueAction(action, steps, speed, 0, amount);
                               return true;
                           });

        // 系统工具
        mcp_server.AddTool("self.electron.stop", "ImmediatelyStop", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               // Clear队列但保持任务常驻
                               xQueueReset(action_queue_);
                               is_action_in_progress_ = false;
                               QueueAction(ACTION_HOME, 1, 1000, 0, 0);
                               return true;
                           });

        mcp_server.AddTool("self.electron.get_status", "Get机器人Status，Return moving 或 idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        // 单个舵机校准工具
        mcp_server.AddTool(
            "self.electron.set_trim",
            "校准单个舵机Position。Settings指定舵机的微调Parameters以调整ElectronBot的初始姿态，Settings将永久Save。"
            "servo_type: 舵机Class型(right_pitch:右臂旋转, right_roll:右臂推拉, left_pitch:左臂旋转, "
            "left_roll:左臂推拉, body:身体, head:头部); "
            "trim_value: 微调值(-30到30度)",
            PropertyList({Property("servo_type", kPropertyTypeString, "right_pitch"),
                          Property("trim_value", kPropertyTypeInteger, 0, -30, 30)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Settings舵机微调: %s = %d度", servo_type.c_str(), trim_value);

                // Get当前所有微调值
                Settings settings("electron_trims", true);
                int right_pitch = settings.GetInt("right_pitch", 0);
                int right_roll = settings.GetInt("right_roll", 0);
                int left_pitch = settings.GetInt("left_pitch", 0);
                int left_roll = settings.GetInt("left_roll", 0);
                int body = settings.GetInt("body", 0);
                int head = settings.GetInt("head", 0);

                // Update指定舵机的微调值
                if (servo_type == "right_pitch") {
                    right_pitch = trim_value;
                    settings.SetInt("right_pitch", right_pitch);
                } else if (servo_type == "right_roll") {
                    right_roll = trim_value;
                    settings.SetInt("right_roll", right_roll);
                } else if (servo_type == "left_pitch") {
                    left_pitch = trim_value;
                    settings.SetInt("left_pitch", left_pitch);
                } else if (servo_type == "left_roll") {
                    left_roll = trim_value;
                    settings.SetInt("left_roll", left_roll);
                } else if (servo_type == "body") {
                    body = trim_value;
                    settings.SetInt("body", body);
                } else if (servo_type == "head") {
                    head = trim_value;
                    settings.SetInt("head", head);
                } else {
                    return "Error：无效的舵机Class型，Please使用: right_pitch, right_roll, left_pitch, "
                           "left_roll, body, head";
                }

                electron_bot_.SetTrims(right_pitch, right_roll, left_pitch, left_roll, body, head);

                QueueAction(ACTION_HOME, 1, 500, 0, 0);

                return "舵机 " + servo_type + " 微调Settings为 " + std::to_string(trim_value) +
                       " 度，Already永久Save";
            });

        mcp_server.AddTool("self.electron.get_trims", "Get当前的舵机微调Settings", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("electron_trims", false);

                               int right_pitch = settings.GetInt("right_pitch", 0);
                               int right_roll = settings.GetInt("right_roll", 0);
                               int left_pitch = settings.GetInt("left_pitch", 0);
                               int left_roll = settings.GetInt("left_roll", 0);
                               int body = settings.GetInt("body", 0);
                               int head = settings.GetInt("head", 0);

                               std::string result =
                                   "{\"right_pitch\":" + std::to_string(right_pitch) +
                                   ",\"right_roll\":" + std::to_string(right_roll) +
                                   ",\"left_pitch\":" + std::to_string(left_pitch) +
                                   ",\"left_roll\":" + std::to_string(left_roll) +
                                   ",\"body\":" + std::to_string(body) +
                                   ",\"head\":" + std::to_string(head) + "}";

                               ESP_LOGI(TAG, "Get微调Settings: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.battery.get_level", "Get机器人BatteryBattery level和ChargeStatus", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });

        ESP_LOGI(TAG, "Electron Bot MCP工具RegisterDone");
    }

    ~ElectronBotController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static ElectronBotController* g_electron_controller = nullptr;

void InitializeElectronBotController() {
    if (g_electron_controller == nullptr) {
        g_electron_controller = new ElectronBotController();
        ESP_LOGI(TAG, "Electron Bot控制器AlreadyInitialize并RegisterMCP工具");
    }
}
