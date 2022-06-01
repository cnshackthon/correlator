#include "frame_subidscan.h"
#include <WiFi.h>
#include <HTTPClient.h>
#define MIN_RSSI -80           // Minimum RSSI [db] needed to perform address comparison
#define SCAN_TIME 10           // seconds
#define LOOP_TIME 5            // seconds
#define NO_DEVICE_TIMER 3 * 60 // seconds

#define MAX_BLE_NUM (MAX_BLE_BTN_NUM - 1)
const uint8_t *kIMGBleLevel[4] = {
    NULL,
    ImageResource_item_icon_wifi_1_32x32,
    ImageResource_item_icon_wifi_2_32x32,
    ImageResource_item_icon_wifi_3_32x32};
bool _update_ble_flag = false;
const char *serverName = "http://192.168.0.4:8081/ndscf/v1/correlation";

void key_ble_cb(epdgui_args_vector_t &args)
{
    if (((EPDGUI_Button *)(args[0]))->GetCustomString() == "_$refresh$_")
    {
        _update_ble_flag = true;
    }
}

Frame_SubIdScan::Frame_SubIdScan(void)
{
    _frame_name = "Frame_SubIdScan";

    for (int i = 0; i < MAX_BLE_BTN_NUM; i++)
    {
        _key_ble[i] = new EPDGUI_Button(4, 100 + i * 60, 532, 61);
        _key_ble[i]->SetHide(true);
        _key_ble[i]->CanvasNormal()->setTextSize(26);
        _key_ble[i]->CanvasNormal()->setTextDatum(CL_DATUM);
        _key_ble[i]->CanvasNormal()->setTextColor(15);
        _key_ble[i]->Bind(EPDGUI_Button::EVENT_RELEASED, nullptr);
        _key_ble[i]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, _key_ble[i]);
        _key_ble[i]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, (void *)(&_is_run));
        _key_ble[i]->Bind(EPDGUI_Button::EVENT_RELEASED, key_ble_cb);
    }

    _language = GetLanguage();
    if (_language == LANGUAGE_JA)
    {
        exitbtn("ホーム");
        _canvas_title->drawString("SubIdScaner", 270, 34);
    }
    else if (_language == LANGUAGE_ZH)
    {
        exitbtn("主页");
        _canvas_title->drawString("SubIdScaner", 270, 34);
    }
    else
    {
        exitbtn("Home");
        _canvas_title->drawString("SubIdScaner", 270, 34);
    }

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, (void *)(&_is_run));
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, &Frame_Base::exit_cb);
}

Frame_SubIdScan::~Frame_SubIdScan(void)
{
    for (int i = 0; i < MAX_BLE_BTN_NUM; i++)
    {
        delete _key_ble[i];
    }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        //  log_e("got BLE device %s\n", advertisedDevice.toString().c_str());
        if (advertisedDevice.haveName())
        {
            String name = advertisedDevice.getName().c_str();
            log_e("MyAdvertisedDeviceCallbacks: get BLE device with name %s\n", advertisedDevice.getName().c_str());
        }
    }
};

void scanCompleteCB(BLEScanResults result)
{

    int count = result.getCount();
    log_e("scanCompleteCB scan: get BLE %d devices\n", count);
    int avaliableSubDevice = 0;
    Frame_SubIdScan *frame = (Frame_SubIdScan *)EPDGUI_GetFrame("Frame_SubIdScan");
    if (frame == NULL)
    {
        frame = new Frame_SubIdScan();
        EPDGUI_AddFrame("Frame_SubIdScan", frame);
        EPDGUI_PushFrame(frame);
    }
    frame->_deviceList.clear();
    for (int i = 0; i < count; i++)
    {

        BLEAdvertisedDevice d = result.getDevice(i);
        if (d.getRSSI() >= MIN_RSSI && d.haveName())
        {

            String name = d.getName().c_str();
            log_e("scanCompleteCB get BLE device : %s, RSSI %d\n", name.c_str(), d.getRSSI());
            if (avaliableSubDevice < MAX_BLE_NUM)
            {
                frame->DrawItem(frame->_key_ble[avaliableSubDevice], name, d.getRSSI());
                frame->_key_ble[avaliableSubDevice]->SetHide(false);
                frame->_key_ble[avaliableSubDevice]->Draw(UPDATE_MODE_A2);
            }
            avaliableSubDevice++;
            frame->_deviceList.push_back(name);
        }
    }

    frame->_key_ble[avaliableSubDevice]->SetCustomString("_$refresh$_");
    frame->_key_ble[avaliableSubDevice]->SetHide(false);
    frame->_key_ble[avaliableSubDevice]->CanvasNormal()->fillCanvas(0);
    frame->_key_ble[avaliableSubDevice]->CanvasNormal()->drawRect(0, 0, 532, 61, 15);
    frame->_key_ble[avaliableSubDevice]->CanvasNormal()->pushImage(15, 14, 32, 32, ImageResource_item_icon_refresh_32x32);

    if (GetLanguage() == LANGUAGE_JA)
    {
        frame->_key_ble[avaliableSubDevice]->CanvasNormal()->drawString("刷新", 58, 35);
    }
    else if (GetLanguage() == LANGUAGE_ZH)
    {
        frame->_key_ble[avaliableSubDevice]->CanvasNormal()->drawString("刷新", 58, 35);
    }
    else
    {
        frame->_key_ble[avaliableSubDevice]->CanvasNormal()->drawString("Refresh", 58, 35);
    }
    *(frame->_key_ble[avaliableSubDevice]->CanvasPressed()) = *(frame->_key_ble[avaliableSubDevice]->CanvasNormal());
    frame->_key_ble[avaliableSubDevice]->CanvasPressed()->ReverseColor();
    frame->_key_ble[avaliableSubDevice]->Draw(UPDATE_MODE_A2);
    M5.EPD.UpdateFull(UPDATE_MODE_GL16);
    // send http message
    String httpRequestData = "{\"subIds\":[";
    for (auto it = frame->_deviceList.begin(); it != frame->_deviceList.end();)
    {
        httpRequestData += "\"";
        httpRequestData += (*it).c_str();
        httpRequestData += "\"";
        if (++it != frame->_deviceList.end())
        {
            httpRequestData += ",";
        }
    }
    httpRequestData += "]}";
    log_e("send http message with body : %s", httpRequestData.c_str());
    // Your Domain name with URL path or IP address with path
    String serverFullName = serverName;
    serverFullName += "/imsi-999559807001001";
    log_e("message server : %s", serverFullName.c_str());
    HTTPClient http;
    if (WiFi.status() != WL_CONNECTED)
    {
        log_e("Wifi Not connected");
        return;
    }
    // const char *serverName = "http://192.168.0.4:8081/ndscf/v1/correlation";
    //     http.begin(client, serverFullName);
    // http.begin(serverFullName);
    http.begin("192.168.0.4", 8081, "/ndscf/v1/correlation/imsi-999559807001001");
    // Specify content-type header
    // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Content-Type", "application/json");

    // Send HTTP POST request
    int httpResponseCode = http.PUT(httpRequestData);

    // If you need an HTTP request with a content type: application/json, use the following:
    // http.addHeader("Content-Type", "application/json");
    // int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");

    // If you need an HTTP request with a content type: text/plain
    // http.addHeader("Content-Type", "text/plain");
    // int httpResponseCode = http.POST("Hello, World!");
    if (httpResponseCode != HTTP_CODE_OK)
    {
        log_e("HTTP Response code: %d\n", httpResponseCode);
    }
    else
    {
        log_e("HTTP success");
    }
    // Free resources
    http.end();
}

int Frame_SubIdScan::scan()
{
    if (_scan_count > 0)
    {
        M5.EPD.WriteFullGram4bpp(GetWallpaper());
        _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
        _key_exit->Draw(UPDATE_MODE_NONE);
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    }
    _scan_count++;

    BLEDevice::init("");

    BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan

    //   pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
    pBLEScan->start(SCAN_TIME, scanCompleteCB, false);

    log_e("scan done");

    //  BLEScanResults foundDevices = pBLEScan->start(10, false);

    //  scanCompleteCB(foundDevices);
    /*
    int count = foundDevices.getCount();
    log_e("scan: get BLE %d devices\n", count);
    for (int i = 0; i < count; i++)
    {

        BLEAdvertisedDevice d = foundDevices.getDevice(i);
        if (d.getRSSI() >= MIN_RSSI)
        {
            String name = d.getName().c_str();
            log_e("get BLE device : %s\n", name.c_str());
        }
    }
    */

    return 0;
}

int Frame_SubIdScan::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    M5.EPD.WriteFullGram4bpp(GetWallpaper());
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);

    for (int i = 0; i < MAX_BLE_BTN_NUM; i++)
    {
        _key_ble[i]->SetHide(true);
        EPDGUI_AddObject(_key_ble[i]);
    }

    EPDGUI_AddObject(_key_exit);

    scan();

    return 3;
}

int Frame_SubIdScan::run()
{
    Frame_Base::run();

    if (_update_ble_flag)
    {
        _update_ble_flag = false;
        for (int i = 0; i < MAX_BLE_BTN_NUM; i++)
        {
            _key_ble[i]->SetHide(true);
        }
        scan();
    }
    return 1;
}

void Frame_SubIdScan::DrawItem(EPDGUI_Button *btn, String ssid, int rssi)
{
    int level = 0;
    if (rssi > -75)
    {
        level = 3;
    }
    else if (rssi > -88)
    {
        level = 2;
    }
    else
    {
        level = 1;
    }
    if (ssid.length() > 22)
    {
        ssid = ssid.substring(0, 22) + "...";
    }
    btn->SetHide(false);
    btn->CanvasNormal()->fillCanvas(0);
    btn->CanvasNormal()->drawRect(0, 0, 532, 61, 15);
    btn->CanvasNormal()->drawString(ssid, 15, 35);
    btn->SetCustomString(ssid);
    btn->CanvasNormal()->pushImage(532 - 15 - 32, 14, 32, 32, kIMGBleLevel[level]);
    *(btn->CanvasPressed()) = *(btn->CanvasNormal());
    btn->CanvasPressed()->ReverseColor();
}
