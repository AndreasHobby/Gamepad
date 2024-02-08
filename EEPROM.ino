//*****************************************************************************************************************
// ** header incldes and installation hints **
//*****************************************************************************************************************
#include <Preferences.h>
//*****************************************************************************************************************
// ** ERPROM storage data **
Preferences Data;
//*****************************************************************************************************************
//*****************************************************************************************************************
// function to initialize data
void DataPreset() {
  // add example config
  SBrick[0].AxisId[0] = 1;
  SBrick[0].AxisAxis[0] = 3;
  SBrick[0].AxisId[1] = 2;
  SBrick[0].AxisAxis[1] = 3;
  SBrick[4].AxisId[0] = 3;
  SBrick[4].AxisAxis[0] = 2;
  SBrick[5].AxisId[0] = 1;
  SBrick[5].AxisAxis[0] = 2;
  LogAdd("EEPROM: data preset done", TFT_GOLD);
}
//*****************************************************************************************************************
// 
void LoadDefaults() {
  // add qualified adresses
  SBrick[0].Address = "90:fd:9f:bc:04:e5"; //free
  SBrick[1].Address = "90:fd:9f:bd:61:cf"; //free
  SBrick[2].Address = "88:6b:0f:77:5d:91"; //bagger
  SBrick[3].Address = "88:6b:0f:43:b3:11"; //bagger
  SBrick[4].Address = "";
  SBrick[5].Address = "";
  SBrick[6].Address = "";
  SBrick[7].Address = "";
  SBrick[0].Type = SBRICK_DEVICE_HTTP;
  SBrick[1].Type = SBRICK_DEVICE_HTTP;
  SBrick[2].Type = SBRICK_DEVICE_HTTP;
  SBrick[3].Type = SBRICK_DEVICE_HTTP;
  SBrick[4].Type = SBRICK_DEVICE_HTTP;
  SBrick[5].Type = SBRICK_DEVICE_HTTP;
  SBrick[6].Type = SBRICK_DEVICE_HTTP;
  SBrick[7].Type = SBRICK_DEVICE_HTTP;
  SBrick[0].IP = 121;
  SBrick[1].IP = 122;
  SBrick[2].IP = 123;
  SBrick[3].IP = 124;
  SBrick[4].IP = 125;
  SBrick[5].IP = 126;
  SBrick[6].IP = 127;
  SBrick[7].IP = 128;
  LogAdd("EEPROM: defaults loaded", TFT_GREEN);
}
//*****************************************************************************************************************
// 
void LoadInit() {
  for (int i=0; i < BLE_MAX; i++) {
    SBrick[i].IP = 0;
    SBrick[i].Char = DEVICE_CHAR_MAX;
    SetChar(i, SBrick[i].Char);
    for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
      SBrick[i].Req[channel] = 0;
      SBrick[i].AxisAxis[channel] = 255;
      SBrick[i].AxisId[channel] = 255;
    }
  }
}
//*****************************************************************************************************************
// 
void DeleteAllEepromData() {
  Data.begin("main", false);
  Data.clear();
  Data.end();
  Data.begin("data", false);
  Data.clear();
  Data.end();
  for (int index=0; index < BLE_MAX; index++) {
    String S = "data"+String(index);
    Data.begin(S.c_str(), false);
    Data.clear();
    Data.end();
  }
  LogAdd("EEPROM: deleted (" + String(Data.freeEntries()) + " free)", TFT_YELLOW);
}
//*****************************************************************************************************************
// 
void EepromInit() {
  LoadInit();
  LoadDefaults();
  LoadEepromDataMain();
  //LoadEepromData(1);
  for (int i=0; i < BLE_MAX; i++)
    DataInit(i);
}
//*****************************************************************************************************************
// 
void LoadEepromDataMain() {
  MainIP = 178;
  MainSwitchIP = 117;
  MainPositionGradient = 64;
  MainWifi = 0x01;
  MainBle = 0x01;
  Data.begin("main", true);
  MainIP = Data.getChar("MainIP", MainIP);
  MainSwitchIP = Data.getChar("MainSwitchIP", MainSwitchIP);
  MainPositionGradient = Data.getChar("MainPositionGradient", MainPositionGradient);
  MainWifi = Data.getChar("MainWifi", MainWifi);
  MainBle = Data.getChar("MainBle", MainBle);
  MainAccel = Data.getChar("MainAccel", MainAccel);
  
  Data.end();
  if (MainBle == 0x01) BLE_RequestScan = true;
}
//*****************************************************************************************************************
// 
void LoadEepromData(int index) {
  LoadInit();
  String S;
  S = "data"+String(index);
  Data.begin(S.c_str(), true);
  for (int i=0; i < BLE_MAX; i++) {
    //S = "Type"+String(i);
    //SBrick[i].Type = Data.getChar(S.c_str(), SBrick[i].Type);
    S = "IP"+String(i);
    SBrick[i].IP = Data.getChar(S.c_str(), SBrick[i].IP);
    S = "Char"+String(i);
    SBrick[i].Char = Data.getChar(S.c_str(), SBrick[i].Char);
    SetChar(i, SBrick[i].Char);
    for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
      S = "Req"+String(i)+String(channel);
      SBrick[i].Req[channel] = Data.getChar(S.c_str(), SBrick[i].Req[channel]);
      S = "AxisId"+String(i)+String(channel);
      SBrick[i].AxisId[channel] = Data.getChar(S.c_str(), SBrick[i].AxisId[channel]);
      S = "AxisAxis"+String(i)+String(channel);
      SBrick[i].AxisAxis[channel] = Data.getChar(S.c_str(), SBrick[i].AxisAxis[channel]);

      //SBrick[i].Req[channel] = 0;
      //SBrick[i].AxisId[channel] = 255;
      //SBrick[i].AxisAxis[channel] = 255;
    }
  }
  LogAdd("EEPROM: (" + String(index) + ")loaded (" + String(Data.freeEntries()) + " free)", TFT_GREEN);
  Data.end();
}
//*****************************************************************************************************************
// 
void SaveEepromDataMain() {
  String S;
  Data.begin("main", false);
  Data.clear();
  if (MainIP != 178) Data.putChar("MainIP", MainIP);
  if (MainSwitchIP != 117) Data.putChar("MainSwitchIP", MainSwitchIP);
  if (MainPositionGradient != 64) Data.putChar("MainPositionGradient", MainPositionGradient);
  Data.putChar("MainWifi", MainWifi);
  Data.putChar("MainBle", MainBle);
  Data.putChar("MainAccel", MainAccel);
  LogAdd("EEPROM: saved (" + String(Data.freeEntries()) + " free)", TFT_GREEN);
  Data.end();
}
//*****************************************************************************************************************
// 
void SaveEepromData(int index) {
  String S;
  S = "data"+String(index);
  Data.begin(S.c_str(), false);
  Data.clear();
  for (int i=0; i < BLE_MAX; i++) {
    S = "IP"+String(i);
    if (SBrick[i].IP != 0) Data.putChar(S.c_str(), SBrick[i].IP);
    S = "Char"+String(i);
    if (SBrick[i].Char != DEVICE_CHAR_MAX) Data.putChar(S.c_str(), SBrick[i].Char);
    S = "Address"+String(i);
    //Data.putString(S.c_str(), SBrick[i].Address);
    for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
      S = "Req"+String(i)+String(channel);
      if (SBrick[i].Req[channel] != 0) Data.putChar(S.c_str(), SBrick[i].Req[channel]);
      S = "AxisId"+String(i)+String(channel);
      if (SBrick[i].AxisId[channel] != 255) Data.putChar(S.c_str(), SBrick[i].AxisId[channel]);
      S = "AxisAxis"+String(i)+String(channel);
      if (SBrick[i].AxisAxis[channel] != 255) Data.putChar(S.c_str(), SBrick[i].AxisAxis[channel]);
    }
  }
  LogAdd("EEPROM: (" + String(index) + ")saved (" + String(Data.freeEntries()) + " free)", TFT_GREEN);
  Data.end();
}
