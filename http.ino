//*****************************************************************************************************************
// ** header incldes and installation hints **
#include <ArduinoOTA.h>                    //url=https://github.com/jandrassy/ArduinoOTA
#include <WiFi.h>                          //built in via ESP32 installation
//#include <AsyncHTTPRequest_Generic.h>    //https://github.com/khoih-prog/AsyncHTTPRequest_Generic
#include <HTTPClient.h>                    //built in via ESP32 installation 
#include "PasswordSettings.h"              //define ssid and password String in  external header file
//*****************************************************************************************************************
// ** HTTP configuration **
#define HTML_REQUEST_CONNECTTIMEOUT  500 //150 => 100
#define WIFI_CONNECTTIMEOUT          10000 //20000
#define WIFI_MAXBLE_DELAY            5000
#define WIFI_SEARCH_TIMEOUT          1000
//*****************************************************************************************************************
// ** HTTP storage data **
unsigned long WiFiConnectTime;
uint32_t HttpLastScanTime = 0;
bool WiFiFirstConnect;
//*****************************************************************************************************************
//*****************************************************************************************************************
// send data
int sendHtmlPower(int id) {
  int Power = SBrick[id].PowerLim[0];
  HTTPClient http;                         //create HTTP client
  http.setConnectTimeout(HTML_REQUEST_CONNECTTIMEOUT);      //set up timeout (to avoid blocking the CPU)
  String S = "http://" + PreIP + String(MainIP) + "." + String(SBrick[id].IP) + "/get";
  for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
    int Power = SBrick[id].PowerLim[channel];
    if (channel == 0) S = S + "?"; else S = S + "&";
    S = S + "Motor"+String(channel+1)+"="+String(Power);
  }
  if (SBrick[id].Char == DEVICE_CHAR_LOC) {
     S = S + "?Motor=" + SBrick[id].PowerLim[0];
  }
  http.begin(S);                                            //requesting power to a device (no connection check done before)
  int httpCode = http.GET();
  if (millis() > SBrick[id].lastVoltageRequest) {
    if (httpCode > 0){
      SBrick[id].lastVoltageRequest = millis() + BLE_VOLTAGE_DELAY;
      if(httpCode == HTTP_CODE_OK){
        String S = http.getString();
        int Pos = S.indexOf("RSSI");
        if (Pos >= 0) {
          String R = S.substring(Pos+7, Pos+9);
          SBrick[id].RSSI = R.toInt();
          SBrick[id].RSSI = -SBrick[id].RSSI;
        }
        Pos = S.indexOf("Voltage");
        if (Pos >= 0) {
          String R = S.substring(Pos+9, Pos+13);
          SBrick[id].Voltage = R.toFloat();
        }
      }
    }
  }
  http.end();                              //free the resources
  return httpCode;                         //return http code
}
//*****************************************************************************************************************
// send data
int sendHtmlSwitch(bool logic, int address, int id) {
  int IP = MainSwitchIP + 1;
  if (address == 1) IP = MainSwitchIP + 2;
  if (address == 2) IP = MainSwitchIP;
  String Switch = "1";
  if (logic)        Switch = "2";
  HTTPClient http;                         //create HTTP client
  http.setConnectTimeout(HTML_REQUEST_CONNECTTIMEOUT);      //set up timeout (to avoid blocking the CPU)
  String S = "http://" + PreIP + String(MainIP) + "." + String(IP) + "/get?Servo" + Switch + "=" + String(id);
  for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
    int Power = SBrick[id].PowerLim[channel];
    if (channel == 0) S = S + "?"; else S = S + "&";
    S = S + "Motor"+String(channel+1)+"="+String(Power);
  }
  http.begin(S);                                            //requesting switch to a device (no connection check done before)
  int httpCode = http.GET();
  if (httpCode > 0){
    if(httpCode == HTTP_CODE_OK){
      String S = http.getString();
      int Pos = S.indexOf("RSSI");
      if (Pos >= 0) {
        String R = S.substring(Pos+7, Pos+9);
        //SBrick[id].RSSI = R.toInt();
        //SBrick[id].RSSI = -SBrick[id].RSSI;
      }
      Pos = S.indexOf("Voltage");
      if (Pos >= 0) {
        String R = S.substring(Pos+9, Pos+13);
        SBrick[id].Voltage = R.toFloat();
      }
    }
  }
  http.end();                              //free the resources
  return httpCode;                         //return http code
}

//*****************************************************************************************************************
// HTTP polling to request data from SBricks or derivates
void WifiCyclicCall() {
  ArduinoOTA.handle();                     //cyclic check OTA requests to allow OTA

  uint32_t time = millis();

  if (HTTP_RequestScan) {
    if (time > HttpLastScanTime){
      HttpLastScanTime = time + WIFI_SEARCH_TIMEOUT;
      //predefine port
      SBrick[7].PowerLim[0] = 63;
      SBrick[7].IP = HTTP_RequestScanIP;
      SBrick[7].Type = SBRICK_DEVICE_HTTP;
      //try to connect to IP
      int returnval = sendHtmlPower(7);
      if (returnval > 0) {
        HttpLastScanTime = time + 100;
        //IP response received
        SBrick[HTTP_RequestScanFound].PowerLim[0] = 0;
        SBrick[HTTP_RequestScanFound].IP = SBrick[7].IP;
        SBrick[HTTP_RequestScanFound].Type = SBRICK_DEVICE_HTTP;
        SBrick[HTTP_RequestScanFound].Mode = SBRICK_MODE_CONNECTED_OK;
        SBrick[HTTP_RequestScanFound].Char = DEVICE_CHAR_LOC;
        SetChar(HTTP_RequestScanFound, SBrick[HTTP_RequestScanFound].Char);
        SBrick[HTTP_RequestScanFound].Req[0] = DEVICE_REQ_TARGET;
        switch (HTTP_RequestScanFound){
          case 0: SBrick[HTTP_RequestScanFound].AxisId[0] = 1; break;
          case 1: SBrick[HTTP_RequestScanFound].AxisId[0] = 2; break;
          case 2: SBrick[HTTP_RequestScanFound].AxisId[0] = 1; break;
          case 3: SBrick[HTTP_RequestScanFound].AxisId[0] = 2; break;
          default: break;
        }
        switch (HTTP_RequestScanFound){
          case 0: SBrick[HTTP_RequestScanFound].AxisAxis[0] = 1; break;
          case 1: SBrick[HTTP_RequestScanFound].AxisAxis[0] = 1; break;
          case 2: SBrick[HTTP_RequestScanFound].AxisAxis[0] = 0; break;
          case 3: SBrick[HTTP_RequestScanFound].AxisAxis[0] = 0; break;
          default: break;
        }
        HTTP_RequestScanFound++;
        LogAdd("HTTP found, IP:" + String(SBrick[7].IP) + " /ID:" + String(HTTP_RequestScanFound), TFT_GOLD);
      }
      HTTP_RequestScanIP++;
      if   ((HTTP_RequestScanFound >= 4)
        // abort condition after 4 IP's found
        || (HTTP_RequestScanIP > 136))
        // abort condition after 16 IP's checked
      {
        HTTP_RequestScan = false;
        HTTP_RequestScanIP = 120;
        HTTP_RequestScanFound = 0;
        SBrick[7].Voltage = 0;
        SBrick[7].RSSI = 0;
      }
    }
  }
}
//*****************************************************************************************************************
// init wifi and setup OTA
void initWifi(void*) {
  while ((BLE_RequestScan) and (millis() < WIFI_MAXBLE_DELAY)) {
    vTaskDelay(100);
  }
  wl_status_t state;
  while (true) {                           //another main loop for wifi only / purpose: asynchron connecting to Wifi
    state = WiFi.status();
    if (state != WL_CONNECTED) {           //no connection
      if (!MainWifi) {
        vTaskDelay(1000);                     //use task delay to allow the CPU to switch to another task
      } else {
        if (state == WL_NO_SHIELD) {         //init case: do WiFi.begin
          LogAdd("WIFI: connecting", TFT_WHITE);
          WiFi.mode(WIFI_STA);
          vTaskDelay(100);
          WiFiConnectTime = millis();       //save time
          WiFi.begin(ssid, password);
          vTaskDelay(100);
        } else if (state == WL_CONNECT_FAILED) {  //WiFi.begin has failed (typically AUTH_FAIL)
          LogAdd("WIFI: disconnecting", TFT_RED);
          WiFi.disconnect(true);
          vTaskDelay(100);
        } else if (state == WL_DISCONNECTED) {    //WiFi.disconnect was done or Router.WiFi got out of range
          if (!WiFiFirstConnect) {           //use help variable to run below code only once
            WiFiFirstConnect = true;
            LogAdd("WIFI: disconnected", TFT_GOLD);
          }
        }
        if (millis() > WiFiConnectTime + WIFI_CONNECTTIMEOUT){
          LogAdd("WIFI: timeout", TFT_GOLD);
          WiFi.disconnect(true);
          vTaskDelay(100);
        }
        vTaskDelay(500);                     //use task delay to allow the CPU to switch to another task
      }
    } else {                               //connection done
      if (WiFiFirstConnect) {              //use help variable to run below code only once
        WiFiFirstConnect = false;
        WiFiConnectTime = millis() - WiFiConnectTime;           //calculate time co connect
        IP = FUN_IpAddress2String(WiFi.localIP());
        LogAdd("WIFI:" + IP + "/" + String(WiFiConnectTime) + "ms)", TFT_GREEN);
        vTaskDelay(100);
        //ArduinoOTA.setHostname("");      //OTA hostname defaults to esp8266/ESP32-[ChipID]
        ArduinoOTA.onStart([]() {          //OTA onStart callback to load new sketch 
          OTAoff = false;
          TftFlashStart();
          String type;
          if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
          } else { // U_FS
            type = "filesystem";
          }
        });
        ArduinoOTA.onEnd([]() {});
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { TftFlashProgress(progress, total); });
        ArduinoOTA.onError([](ota_error_t error) {
          Serial.printf("Error[%u]: ", error);
          if (error == OTA_AUTH_ERROR) { LogAdd("OTA: auth failed", TFT_RED); } else if (error == OTA_BEGIN_ERROR) { LogAdd("OTA: begin failed", TFT_RED); } else if (error == OTA_CONNECT_ERROR) { LogAdd("OTA: connect failed", TFT_RED); }
          else if (error == OTA_RECEIVE_ERROR) { LogAdd("OTA: receive failed", TFT_RED); } else if (error == OTA_END_ERROR) { LogAdd("OTA: end failed", TFT_RED); };
        });                                //OTA show error /only for debugging)
        ArduinoOTA.begin();                //OTA start
        LogAdd("WIFI: connected @RSSI:" + String(WiFi.RSSI()), TFT_GREEN);
      }
      if (!MainWifi) WiFi.disconnect(true);
      vTaskDelay(1000);                    //use task delay to allow the CPU to switch to another task
    }
  }
}
//*****************************************************************************************************************
// helper function to convert IP address in string
String FUN_IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + String(ipAddress[1]) + String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]);
}
