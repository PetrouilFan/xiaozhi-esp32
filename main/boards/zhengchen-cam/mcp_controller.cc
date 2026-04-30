#include <cJSON.h>
#include <esp_log.h>

#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "sdkconfig.h"
#include "settings.h"
#include "display.h"

#define TAG "MCPController"

class MCPController {
public:
    MCPController() {
        RegisterMcpTools();
        ESP_LOGI(TAG, "Register MCP tools");
    }

	void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();
		ESP_LOGI(TAG, "Starting MCP tools registration...");

	mcp_server.AddTool(
        "self.AEC.set_mode", 
        "SettingsAEC对话打断Mode。当用户意图切换对话打断Mode时或者用户觉得ai对话容易被打断时或者用户觉得CannotImplement对话打断时都使用此工具。\n"
        "Parameters：\n"
        "   `mode`: 对话打断Mode，Optional values只有`kAecOff`(Close）和`kAecOnDeviceSide`（Open/Enable）\n"
        "Return值：\n"
        "   FeedbackStatusInfo，不需要Confirm，Immediately播报相关数据\n",
        PropertyList({
            Property("mode", kPropertyTypeString)
        }), 
        [](const PropertyList& properties) -> ReturnValue {
            auto mode = properties["mode"].value<std::string>();
            auto& app = Application::GetInstance();
            vTaskDelay(pdMS_TO_TICKS(2000));
            if (mode == "kAecOff") {
                app.SetAecMode(kAecOff);
                return "{\"success\": true, \"message\": \"AEC interrupt mode disabled\"}";
            }else {
                auto& board = Board::GetInstance();
                app.SetAecMode(kAecOnDeviceSide);
                
                return "{\"success\": true, \"message\": \"AEC interrupt mode enabled\"}";
            }
        }
    );

    mcp_server.AddTool(
        "self.AEC.get_mode",
        "GetAEC对话打断ModeStatus。当用户意图Get对话打断ModeStatus时使用此工具。\n"
        "Return值：\n"
        "   FeedbackStatusInfo，不需要Confirm，Immediately播报相关数据\n",
        PropertyList(),  
        [](const PropertyList&) -> ReturnValue {
            auto& app = Application::GetInstance();
            const bool is_currently_off = (app.GetAecMode() == kAecOff);
           if (is_currently_off) {
                return "{\"success\": true, \"message\": \"AEC interrupt mode is off\"}";
            }else {
                return "{\"success\": true, \"message\": \"AEC interrupt mode is on\"}";
            }
        }
    );
	
    mcp_server.AddTool(
        "self.res.esp_restart",
        "Restart设备。当用户意图Restart设备时使用此工具。\n",
        PropertyList(),  
        [](const PropertyList&) -> ReturnValue {
            vTaskDelay(pdMS_TO_TICKS(1000));
            // Reboot the device
            esp_restart();
            return true;
        }
    );

        ESP_LOGI(TAG, "MCP tools registration complete");
    }

};

static MCPController* g_mcp_controller = nullptr;

void InitializeMCPController() {
    if (g_mcp_controller == nullptr) {
        g_mcp_controller = new MCPController();
        ESP_LOGI(TAG, "Register MCP tools");
    }
}