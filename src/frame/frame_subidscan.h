#ifndef _Frame_SubIdScan_H_
#define _Frame_SubIdScan_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#define MAX_BLE_BTN_NUM 14
class Frame_SubIdScan : public Frame_Base
{
public:
    Frame_SubIdScan();
    ~Frame_SubIdScan();
    int init(epdgui_args_vector_t &args);
    int scan();
    int run();
    void DrawItem(EPDGUI_Button *btn, String ssid, int rssi);

    EPDGUI_Button *_key_ble[MAX_BLE_BTN_NUM];
    uint8_t _language;
    uint32_t _scan_count = 0;
    std::vector<String> _deviceList;
};

#endif //_FRAME_SETTING_H_