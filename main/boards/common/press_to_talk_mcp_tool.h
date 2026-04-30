#ifndef PRESS_TO_TALK_MCP_TOOL_H
#define PRESS_TO_TALK_MCP_TOOL_H

#include "mcp_server.h"
#include "settings.h"

// 可复用的按键说话ModeMCP工具Class
class PressToTalkMcpTool {
private:
    bool press_to_talk_enabled_;

public:
    PressToTalkMcpTool();
    
    // Initialize工具，Register到MCPServer
    void Initialize();
    
    // Get当前按键说话ModeStatus
    bool IsPressToTalkEnabled() const;

private:
    // MCP工具的回调Functions
    ReturnValue HandleSetPressToTalk(const PropertyList& properties);
    
    // 内部Method：Settingspress to talkStatus并Save到Settings
    void SetPressToTalkEnabled(bool enabled);
};

#endif // PRESS_TO_TALK_MCP_TOOL_H 