#ifndef DUAL_NETWORK_BOARD_H
#define DUAL_NETWORK_BOARD_H

#include "board.h"
#include "wifi_board.h"
#include "ml307_board.h"
#include <memory>

//enum NetworkType
enum class NetworkType {
    WIFI,
    ML307
};

// 双Network板卡Class，可以在WiFi和ML307之间切换
class DualNetworkBoard : public Board {
private:
    // 使用基Class指针存储当前活动的板卡
    std::unique_ptr<Board> current_board_;
    NetworkType network_type_ = NetworkType::ML307;  // Default to ML307

    // ML307的引脚Configuration
    gpio_num_t ml307_tx_pin_;
    gpio_num_t ml307_rx_pin_;
    gpio_num_t ml307_dtr_pin_;
    
    // 从SettingsLoadNetworkClass型
    NetworkType LoadNetworkTypeFromSettings(int32_t default_net_type);
    
    // SaveNetworkClass型到Settings
    void SaveNetworkTypeToSettings(NetworkType type);

    // Initialize当前NetworkClass型对应的板卡
    void InitializeCurrentBoard();
 
public:
    DualNetworkBoard(gpio_num_t ml307_tx_pin, gpio_num_t ml307_rx_pin, gpio_num_t ml307_dtr_pin = GPIO_NUM_NC, int32_t default_net_type = 1);
    virtual ~DualNetworkBoard() = default;
 
    // 切换NetworkClass型
    void SwitchNetworkType();
    
    // Get当前NetworkClass型
    NetworkType GetNetworkType() const { return network_type_; }
    
    // Get当前活动的板卡Reference
    Board& GetCurrentBoard() const { return *current_board_; }
    
    // 重写Board接口
    virtual std::string GetBoardType() override;
    virtual void StartNetwork() override;
    virtual void SetNetworkEventCallback(NetworkEventCallback callback) override;
    virtual NetworkInterface* GetNetwork() override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override;
    virtual std::string GetBoardJson() override;
    virtual std::string GetDeviceStatusJson() override;
};

#endif // DUAL_NETWORK_BOARD_H 