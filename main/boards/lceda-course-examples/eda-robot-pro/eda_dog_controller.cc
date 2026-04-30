/*
    EDA机器狗控制器 - MCP协议Version
*/

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "application.h"
#include "board.h"
#include "config.h"
#include "eda_dog_movements.h"
#include "mcp_server.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "EDARobotDogController"

class EDARobotDogController {
private:
  EDARobotDog dog_;
  TaskHandle_t action_task_handle_ = nullptr;
  QueueHandle_t action_queue_;
  bool is_action_in_progress_ = false;

  struct DogActionParams {
    int action_type;
    int steps;
    int speed;
    int direction;
    int height;
  };

  enum ActionType {
    ACTION_WALK = 1,
    ACTION_TURN = 2,
    ACTION_SIT = 3,
    ACTION_STAND = 4,
    ACTION_STRETCH = 5,
    ACTION_SHAKE = 6,
    ACTION_LIFT_LEFT_FRONT = 7,
    ACTION_LIFT_LEFT_REAR = 8,
    ACTION_LIFT_RIGHT_FRONT = 9,
    ACTION_LIFT_RIGHT_REAR = 10,
    ACTION_HOME = 11
  };

  static void ActionTask(void *arg) {
    EDARobotDogController *controller = static_cast<EDARobotDogController *>(arg);
    DogActionParams params;
    controller->dog_.AttachServos();

    while (true) {
      if (xQueueReceive(controller->action_queue_, &params,
                        pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_LOGI(TAG, "执行Action: %d", params.action_type);
        controller->is_action_in_progress_ = true;

        switch (params.action_type) {
        case ACTION_WALK:
          controller->dog_.Walk(params.steps, params.speed, params.direction);
          break;
        case ACTION_TURN:
          controller->dog_.Turn(params.steps, params.speed, params.direction);
          break;
        case ACTION_SIT:
          controller->dog_.Sit(params.speed);
          break;
        case ACTION_STAND:
          controller->dog_.Stand(params.speed);
          break;
        case ACTION_STRETCH:
          controller->dog_.Stretch(params.speed);
          break;
        case ACTION_SHAKE:
          controller->dog_.Shake(params.speed);
          break;
        case ACTION_LIFT_LEFT_FRONT:
          controller->dog_.LiftLeftFrontLeg(params.speed, params.height);
          break;
        case ACTION_LIFT_LEFT_REAR:
          controller->dog_.LiftLeftRearLeg(params.speed, params.height);
          break;
        case ACTION_LIFT_RIGHT_FRONT:
          controller->dog_.LiftRightFrontLeg(params.speed, params.height);
          break;
        case ACTION_LIFT_RIGHT_REAR:
          controller->dog_.LiftRightRearLeg(params.speed, params.height);
          break;
        case ACTION_HOME:
          controller->dog_.Home();
          break;
        }

        if (params.action_type != ACTION_HOME &&
            params.action_type != ACTION_SIT) {
          controller->dog_.Home();
        }
        controller->is_action_in_progress_ = false;
        vTaskDelay(pdMS_TO_TICKS(20));
      }
    }
  }

  void StartActionTaskIfNeeded() {
    if (action_task_handle_ == nullptr) {
      xTaskCreate(ActionTask, "dog_action", 1024 * 3, this,
                  configMAX_PRIORITIES - 1, &action_task_handle_);
    }
  }

  void QueueAction(int action_type, int steps, int speed, int direction,
                   int height) {
    ESP_LOGI(TAG, "Action控制: Class型=%d, 步数=%d, Speed=%d, Direction=%d, Height=%d",
             action_type, steps, speed, direction, height);

    DogActionParams params = {action_type, steps, speed, direction, height};
    xQueueSend(action_queue_, &params, portMAX_DELAY);
    StartActionTaskIfNeeded();
  }

  void LoadTrimsFromNVS() {
    Settings settings("dog_trims", false);

    int left_front_leg = settings.GetInt("left_front_leg", 0);
    int left_rear_leg = settings.GetInt("left_rear_leg", 0);
    int right_front_leg = settings.GetInt("right_front_leg", 0);
    int right_rear_leg = settings.GetInt("right_rear_leg", 0);

    ESP_LOGI(TAG,
             "从NVSLoad微调Settings: 左前腿=%d, 左后腿=%d, 右前腿=%d, 右后腿=%d",
             left_front_leg, left_rear_leg, right_front_leg, right_rear_leg);

    dog_.SetTrims(left_front_leg, left_rear_leg, right_front_leg,
                  right_rear_leg);
  }

public:
  EDARobotDogController() {
    dog_.Init(LEFT_FRONT_LEG_PIN, LEFT_REAR_LEG_PIN, RIGHT_FRONT_LEG_PIN,
              RIGHT_REAR_LEG_PIN);

    ESP_LOGI(TAG, "EDA机器狗InitializeDone");

    LoadTrimsFromNVS();

    action_queue_ = xQueueCreate(10, sizeof(DogActionParams));

    QueueAction(ACTION_HOME, 1, 1000, 0, 0);

    RegisterMcpTools();
  }

  void RegisterMcpTools() {
    auto &mcp_server = McpServer::GetInstance();

    ESP_LOGI(TAG, "Starting MCP tools registration...");

    // 基础MoveAction
    mcp_server.AddTool(
        "self.dog.walk",
        "行走。steps: 行走步数(1-100); speed: "
        "行走Speed(500-2000，数值越小越快); "
        "direction: 行走Direction(-1=后退, 1=前进)",
        PropertyList({Property("steps", kPropertyTypeInteger, 4, 1, 100),
                      Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                      Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int steps = properties["steps"].value<int>();
          int speed = properties["speed"].value<int>();
          int direction = properties["direction"].value<int>();
          QueueAction(ACTION_WALK, steps, speed, direction, 0);
          return true;
        });

    mcp_server.AddTool(
        "self.dog.turn",
        "转身。steps: 转身步数(1-100); speed: "
        "转身Speed(500-2000，数值越小越快); "
        "direction: 转身Direction(1=左转, -1=右转)",
        PropertyList({Property("steps", kPropertyTypeInteger, 4, 1, 100),
                      Property("speed", kPropertyTypeInteger, 2000, 500, 2000),
                      Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int steps = properties["steps"].value<int>();
          int speed = properties["speed"].value<int>();
          int direction = properties["direction"].value<int>();
          QueueAction(ACTION_TURN, steps, speed, direction, 0);
          return true;
        });

    // 姿态Action
    mcp_server.AddTool("self.dog.sit",
                       "坐下。speed: 坐下Speed(500-2000，数值越小越快)",
                       PropertyList({Property("speed", kPropertyTypeInteger,
                                              1500, 500, 2000)}),
                       [this](const PropertyList &properties) -> ReturnValue {
                         int speed = properties["speed"].value<int>();
                         QueueAction(ACTION_SIT, 1, speed, 0, 0);
                         return true;
                       });

    mcp_server.AddTool("self.dog.stand",
                       "站立。speed: 站立Speed(500-2000，数值越小越快)",
                       PropertyList({Property("speed", kPropertyTypeInteger,
                                              1500, 500, 2000)}),
                       [this](const PropertyList &properties) -> ReturnValue {
                         int speed = properties["speed"].value<int>();
                         QueueAction(ACTION_STAND, 1, speed, 0, 0);
                         return true;
                       });

    mcp_server.AddTool("self.dog.stretch",
                       "伸展。speed: 伸展Speed(500-2000，数值越小越快)",
                       PropertyList({Property("speed", kPropertyTypeInteger,
                                              2000, 500, 2000)}),
                       [this](const PropertyList &properties) -> ReturnValue {
                         int speed = properties["speed"].value<int>();
                         QueueAction(ACTION_STRETCH, 1, speed, 0, 0);
                         return true;
                       });

    mcp_server.AddTool("self.dog.shake",
                       "摇摆。speed: 摇摆Speed(500-2000，数值越小越快)",
                       PropertyList({Property("speed", kPropertyTypeInteger,
                                              1000, 500, 2000)}),
                       [this](const PropertyList &properties) -> ReturnValue {
                         int speed = properties["speed"].value<int>();
                         QueueAction(ACTION_SHAKE, 1, speed, 0, 0);
                         return true;
                       });

    // 单腿抬起Action
    mcp_server.AddTool(
        "self.dog.lift_left_front_leg",
        "抬起左前腿。speed: ActionSpeed(500-2000，数值越小越快); height: "
        "抬起Height(10-90度)",
        PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                      Property("height", kPropertyTypeInteger, 45, 10, 90)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int speed = properties["speed"].value<int>();
          int height = properties["height"].value<int>();
          QueueAction(ACTION_LIFT_LEFT_FRONT, 1, speed, 0, height);
          return true;
        });

    mcp_server.AddTool(
        "self.dog.lift_left_rear_leg",
        "抬起左后腿。speed: ActionSpeed(500-2000，数值越小越快); height: "
        "抬起Height(10-90度)",
        PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                      Property("height", kPropertyTypeInteger, 45, 10, 90)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int speed = properties["speed"].value<int>();
          int height = properties["height"].value<int>();
          QueueAction(ACTION_LIFT_LEFT_REAR, 1, speed, 0, height);
          return true;
        });

    mcp_server.AddTool(
        "self.dog.lift_right_front_leg",
        "抬起右前腿。speed: ActionSpeed(500-2000，数值越小越快); height: "
        "抬起Height(10-90度)",
        PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                      Property("height", kPropertyTypeInteger, 45, 10, 90)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int speed = properties["speed"].value<int>();
          int height = properties["height"].value<int>();
          QueueAction(ACTION_LIFT_RIGHT_FRONT, 1, speed, 0, height);
          return true;
        });

    mcp_server.AddTool(
        "self.dog.lift_right_rear_leg",
        "抬起右后腿。speed: ActionSpeed(500-2000，数值越小越快); height: "
        "抬起Height(10-90度)",
        PropertyList({Property("speed", kPropertyTypeInteger, 1000, 500, 2000),
                      Property("height", kPropertyTypeInteger, 45, 10, 90)}),
        [this](const PropertyList &properties) -> ReturnValue {
          int speed = properties["speed"].value<int>();
          int height = properties["height"].value<int>();
          QueueAction(ACTION_LIFT_RIGHT_REAR, 1, speed, 0, height);
          return true;
        });

    // 系统工具
    mcp_server.AddTool("self.dog.stop", "ImmediatelyStop", PropertyList(),
                       [this](const PropertyList &properties) -> ReturnValue {
                         if (action_task_handle_ != nullptr) {
                           vTaskDelete(action_task_handle_);
                           action_task_handle_ = nullptr;
                         }
                         is_action_in_progress_ = false;
                         xQueueReset(action_queue_);

                         QueueAction(ACTION_HOME, 1, 1000, 0, 0);
                         return true;
                       });

    mcp_server.AddTool(
        "self.dog.set_trim",
        "校准单个舵机Position。Settings指定舵机的微调Parameters以调整机器狗的初始站立姿态，设"
        "置将永久Save。"
        "servo_type: "
        "舵机Class型(left_front_leg/left_rear_leg/right_front_leg/"
        "right_rear_leg); "
        "trim_value: 微调值(-50到50度)",
        PropertyList(
            {Property("servo_type", kPropertyTypeString, "left_front_leg"),
             Property("trim_value", kPropertyTypeInteger, 0, -50, 50)}),
        [this](const PropertyList &properties) -> ReturnValue {
          std::string servo_type =
              properties["servo_type"].value<std::string>();
          int trim_value = properties["trim_value"].value<int>();

          ESP_LOGI(TAG, "Settings舵机微调: %s = %d度", servo_type.c_str(),
                   trim_value);

          // Get当前所有微调值
          Settings settings("dog_trims", true);
          int left_front_leg = settings.GetInt("left_front_leg", 0);
          int left_rear_leg = settings.GetInt("left_rear_leg", 0);
          int right_front_leg = settings.GetInt("right_front_leg", 0);
          int right_rear_leg = settings.GetInt("right_rear_leg", 0);

          // Update指定舵机的微调值
          if (servo_type == "left_front_leg") {
            left_front_leg = trim_value;
            settings.SetInt("left_front_leg", left_front_leg);
          } else if (servo_type == "left_rear_leg") {
            left_rear_leg = trim_value;
            settings.SetInt("left_rear_leg", left_rear_leg);
          } else if (servo_type == "right_front_leg") {
            right_front_leg = trim_value;
            settings.SetInt("right_front_leg", right_front_leg);
          } else if (servo_type == "right_rear_leg") {
            right_rear_leg = trim_value;
            settings.SetInt("right_rear_leg", right_rear_leg);
          } else {
            return "Error：无效的舵机Class型，Please使用: left_front_leg, "
                   "left_rear_leg, right_front_leg, right_rear_leg";
          }

          dog_.SetTrims(left_front_leg, left_rear_leg, right_front_leg,
                        right_rear_leg);

          QueueAction(ACTION_HOME, 1, 500, 0, 0);

          return "舵机 " + servo_type + " 微调Settings为 " +
                 std::to_string(trim_value) + " 度，Already永久Save";
        });

    mcp_server.AddTool(
        "self.dog.get_trims", "Get当前的舵机微调Settings", PropertyList(),
        [this](const PropertyList &properties) -> ReturnValue {
          Settings settings("dog_trims", false);

          int left_front_leg = settings.GetInt("left_front_leg", 0);
          int left_rear_leg = settings.GetInt("left_rear_leg", 0);
          int right_front_leg = settings.GetInt("right_front_leg", 0);
          int right_rear_leg = settings.GetInt("right_rear_leg", 0);

          std::string result =
              "{\"left_front_leg\":" + std::to_string(left_front_leg) +
              ",\"left_rear_leg\":" + std::to_string(left_rear_leg) +
              ",\"right_front_leg\":" + std::to_string(right_front_leg) +
              ",\"right_rear_leg\":" + std::to_string(right_rear_leg) + "}";

          ESP_LOGI(TAG, "Get微调Settings: %s", result.c_str());
          return result;
        });

    mcp_server.AddTool("self.dog.get_status",
                       "Get机器狗Status，Return moving 或 idle", PropertyList(),
                       [this](const PropertyList &properties) -> ReturnValue {
                         return is_action_in_progress_ ? "moving" : "idle";
                       });

    ESP_LOGI(TAG, "MCP tools registration complete");
  }

  ~EDARobotDogController() {
    if (action_task_handle_ != nullptr) {
      vTaskDelete(action_task_handle_);
      action_task_handle_ = nullptr;
    }
    vQueueDelete(action_queue_);
  }
};

static EDARobotDogController *g_dog_controller = nullptr;

void InitializeEDARobotDogController() {
  if (g_dog_controller == nullptr) {
    g_dog_controller = new EDARobotDogController();
    ESP_LOGI(TAG, "EDA机器狗控制器AlreadyInitialize并RegisterMCP工具");
  }
}