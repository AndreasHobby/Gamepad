/* (c)2021 Andreas' Hobby
 * Gamepad v1.0
*/
//*****************************************************************************************************************
// ** header incldes and installation hints **
//install ESP32 via Boardverwalter           url=https://dl.espressif.com/dl/package_esp32_index.json
#include "declarations.h"                    //extern declarations
// Board: ESP32 Dev Module
// Partition Scheme: Minimal SPIFFS
//*****************************************************************************************************************
//*****************************************************************************************************************
// 
void LogAdd(String text, int color) {
  Serial.println(text);
  for (int i=0; i < LOG_MAX-1; i++) {   //shift ringbuffer
    LogText[i] = LogText[i+1];
    LogColor[i] = LogColor[i+1];
  }
  LogText[LOG_MAX-1] = text;            //set new log data
  LogColor[LOG_MAX-1] = color;
  TftLogUpdateRequest = true;           //request clear screeen
  LogPosition = LOG_MAX - LOG_MAX_SHOW; //reset log position
}
//*****************************************************************************************************************
// 
String StringFormat(int value, int digit) {
  String result = String(abs(value));
  if (digit == 3) {
    if (abs(value) < 100) result = "0" + result;
  }
  if (digit >= 2) {
    if (abs(value) < 10) result = "0" + result;  
  }
  if (value < 0) result = "-" + result;  
  return result;
}
//*****************************************************************************************************************
// stop function
void StopAll() {
  for (int i=0; i < BLE_MAX; i++)
    if (SBrick[i].Mode >= SBRICK_MODE_CONNECTED_FAIL)
      for (int channel=0; channel < BLE_CHANNEL_MAX; channel++)
        RequestPowerDirect(0, i, channel);
  LogAdd("BLE/HTML: stopped all", TFT_WHITE);
}
//*****************************************************************************************************************
// 
void SetChar(int index, int _char) {
  int _grad = PreGrad[SBrick[EditModeId].Char];
  int _min = PreMin[SBrick[EditModeId].Char];
  int _max = PreMax[SBrick[EditModeId].Char];
  SBrick[index].Gradient = _grad;
  SBrick[index].Min = _min;
  SBrick[index].Max = _max;
  //LogAdd("i:"+String(index+1)+"i:"+String(_grad)+"i:"+String(_min)+"i:"+String(_max), TFT_WHITE);
}
//*****************************************************************************************************************
// keypad callback function
void ChangeMainMenu(int id) {
  if (ControlMainMenu != id) {
    ControlMainMenuLast = ControlMainMenu;
    ControlMainMenu = id;
    EditMode = 0;
    ControlMapAxisId = 255;
    ControlMapAxisAxis = 255;
    TftDrawMenu(ControlMainMenu);
  }
}
int OverwriteNumber(int Number, int Key, int id) {
  int f[3];
  f[0] = 100;  //first  digit
  f[1] = 10;   //second digit
  f[2] = 1;    //third  digit
  int t[3];
  t[0] = Number / f[0];
  t[0] = t[0] * f[0];
  t[1] = (Number - t[0]) / f[1];
  t[1] = t[1] * f[1];
  t[2] = Number - t[0] - t[1]; // / f[2] not needed because 1
  return Number - t[id] + Key*f[id];
}
//*****************************************************************************************************************
// keypad callback function
void KeypadCallbackOnChange(int id, int value) {
  switch(value){
    case  16:
    case  17: break;
    case  3:  //A
      if (ControlMainMenu == MAIN_LOG) ChangeMainMenu(MAIN_INFO); else ChangeMainMenu(MAIN_LOG); break;
    case  7:  //B
      if (ControlMainMenu == MAIN_MAP) ChangeMainMenu(MAIN_LOAD);
             else if (ControlMainMenu == MAIN_LOAD) ChangeMainMenu(MAIN_SAVE);
             else ChangeMainMenu(MAIN_MAP);
             break;
    case 11:  //C
      ChangeMainMenu(MAIN_SWITCH); EditNumber = 0; EditNumberMode = 1; break;
    case 15:  //D
      if (ControlMainMenu == MAIN_RUN) ChangeMainMenu(MAIN_RUNTRAIN);
             else if (ControlMainMenu == MAIN_RUNTRAIN) ChangeMainMenu(MAIN_RUNMULTI);
             else ChangeMainMenu(MAIN_RUN);
             LastControlledId = EditModeId;
             break;
    case 12: { //*
      if (ControlMainMenu != MAIN_NEWDEVICE) StopAll();
      if (EditMode > 0) {
        if (EditNumberMode > 0) EditNumberMode--;
      }
      if (EditNumberMode == 0) EditMode = max(EditMode-1, 0);
      if (ControlMainMenu == MAIN_MAP) {
        ControlMapAxisId = 255;
        ControlMapAxisAxis = 255;
      }
      EditNumber = 0;
      if (ControlMainMenu == MAIN_OPTIONS) {
        if (EditMode == 1) EditNumber = MainIP;
        if (EditMode == 2) EditNumber = MainSwitchIP;
        if (EditMode == 3) EditNumber = MainPositionGradient;
        if (EditMode == 4) EditNumber = MainWifi;
        if (EditMode == 5) EditNumber = MainBle;
        if (EditMode == 5) EditNumber = MainAccel;
      }
      if (ControlMainMenu == MAIN_NEWDEVICE) EditMode = 1;
      break;
    }
    case 14: { //#
      switch (ControlMainMenu){
        case MAIN_OPTIONS:
          if (EditMode == 0) EditNumber = MainIP;
          if (EditMode == 1) {
            if (MainIP != EditNumber) LogAdd("CONF: IP changed", TFT_GREEN);
            MainIP = min(EditNumber, 255);
            EditNumber = MainSwitchIP;
          }
          if (EditMode == 2) {
            if (MainSwitchIP != EditNumber) LogAdd("CONF: MainSwitchIP changed", TFT_GREEN);
            MainSwitchIP = min(EditNumber, 255);
            EditNumber = MainPositionGradient;
          }
          if (EditMode == 3) {
            if (MainPositionGradient != EditNumber) LogAdd("CONF: gradient changed", TFT_GREEN);
            MainPositionGradient = constrain(EditNumber, 1, 255);
            EditNumber = MainWifi;
          }
          if (EditMode == 4) {
            if (MainWifi != EditNumber) LogAdd("CONF: Wifi mode changed", TFT_GREEN);
            MainWifi = constrain(EditNumber, 0, 1);
            EditNumber = MainBle;
          }
          if (EditMode == 5) {
            if (MainBle != EditNumber) LogAdd("CONF: BLE mode changed", TFT_GREEN);
            MainBle = constrain(EditNumber, 0, 1);
          }
          if (EditMode == 6) {
            if (MainAccel != EditNumber) LogAdd("CONF: Accel mode changed", TFT_GREEN);
            MainAccel = constrain(EditNumber, 0, 1);
            SaveEepromDataMain();
          }
          break;
        case MAIN_MAP:
          if (EditMode == 1) {
            ControlMapAxisId = SBrick[EditModeId].AxisId[EditModeChannel];
            ControlMapAxisAxis = SBrick[EditModeId].AxisAxis[EditModeChannel];
            ControlMapReq = SBrick[EditModeId].Req[EditModeChannel];
            Sensor a;
            a.x = 0;
            a.y = 0;
            a.button = false;
            for (int j=0; j < 4; j++) {
              if (j != ControlMapAxisId) TftPrintAnalogStickInfo(j, a, -1);
            }
            if (ControlMapAxisId < 255)   TftPrintAnalogStickInfo(ControlMapAxisId, a, ControlMapAxisAxis);
          }
          if ((EditMode == 2) and (EditModeId >= 0) and (EditModeId < BLE_MAX) and (EditModeChannel >= 0) and (EditModeChannel < BLE_CHANNEL_MAX)) {
            if ((ControlMapReq > 2) or (ControlMapAxisId == 255)) ControlMapReq = 0;
            SBrick[EditModeId].AxisId[EditModeChannel] = ControlMapAxisId;
            SBrick[EditModeId].AxisAxis[EditModeChannel] = ControlMapAxisAxis;
            SBrick[EditModeId].Req[EditModeChannel] = ControlMapReq;
            ControlMapAxisId = 255;
            ControlMapAxisAxis = 255;
            EditNumber = SBrick[EditModeId].Char;
          }
          if (EditMode == 3) {
            SBrick[EditModeId].Char = constrain(EditNumber, 0, 2);
            SetChar(EditModeId, SBrick[EditModeId].Char);
            EditNumber = SBrick[EditModeId].IP;
          }
          if (EditMode == 4) {
            SBrick[EditModeId].IP = min(EditNumber, 255);
            if (SBrick[EditModeId].IP > 0) SBrick[EditModeId].Mode = SBRICK_MODE_CONNECTED;
          }
          break;
        case MAIN_SWITCH:
          switch (EditMode) {
            case 0:
              SwitchState[EditNumber] = !SwitchState[EditNumber];
              int address = EditNumber >> 4;
              int id = EditNumber & 0x0F;
              LogAdd( "SWI: " + String(EditNumber) + " a:" +  String(address) + " nr:" + String(id) + " val:" + String(SwitchState[EditNumber]), TFT_WHITE);
              sendHtmlSwitch(SwitchState[EditNumber], address, id);
              break;
          }
          break;
      }
      if ((ControlMainMenu == MAIN_MAP) and (EditMode == 2) and (EditModeChannel < BLE_CHANNEL_MAX-1)) {
        EditModeChannel++;
        ControlMapAxisId = SBrick[EditModeId].AxisId[EditModeChannel];
        ControlMapAxisAxis = SBrick[EditModeId].AxisAxis[EditModeChannel];
        ControlMapReq = SBrick[EditModeId].Req[EditModeChannel];
      } else {
        EditMode++;
        EditModeChannel = 0;
      }
      EditNumberMode = 0;
      switch (ControlMainMenu){
        case MAIN_NEWDEVICE:
          if (EditMode > 1){
            EditMode = 1;
            if (EditModeId+1 >= ConnectCount) {
              BLE_Takeover = true;
              ChangeMainMenu(ControlMainMenuLast);
            } else {
              EditModeId++;
            }
          }
          break;
        case MAIN_LOG:
        case MAIN_INFO:
          ChangeMainMenu(MAIN_OPTIONS);
          break;
        case MAIN_OPTIONS:
          if (EditMode > 6) EditMode = 0;
          break;
        case MAIN_MAP:
          if ((EditMode > 3) and (SBrick[EditModeId].Type != SBRICK_DEVICE_HTTP)) EditMode = 0;
          if (EditMode > 4) EditMode = 0;
          break;
        case MAIN_SWITCH:
          EditMode = 0;
          EditNumberMode = 1;
          break;
        case MAIN_RUN:
        case MAIN_RUNTRAIN:
          if (EditMode > 1) EditMode = 0;
          break;
        case MAIN_RUNMULTI:
          MultiMode2 = !MultiMode2;
          EditMode = 0;
          break;
      }
      break;
    }
    default: {
      //all numbers 0..9
      int key = int(keys[value]) - 48;
      switch (ControlMainMenu) {
        case MAIN_NEWDEVICE:
              switch (EditMode) {
                case 0:
                  EditModeId = constrain(key-1, 0, BLE_MAX-1);
                  break;
                case 1:
                  ConnectId[EditModeId] = constrain(key, 1, BLE_MAX);
                  break;
              }
              break;
        case MAIN_LOG:
              if (key == 2) { LogPosition--; TftLogUpdateRequest = true; }
              if (key == 8) { LogPosition++; TftLogUpdateRequest = true; }
              LogPosition = constrain(LogPosition, 0, LOG_MAX-LOG_MAX_SHOW);
              break;
        case MAIN_INFO:
              if (key == 6) {
                HTTP_RequestScan = true;
                ChangeMainMenu(MAIN_LOG);
              }
              if (key == 7) {
                BLE_RequestScan = true;
                ChangeMainMenu(MAIN_LOG);
              }
              if (key == 8) {
                ChangeMainMenu(MAIN_TRAININFO);
              }
              EditNumberMode = 0;
              break;
        case MAIN_LOAD:
              if (key == 0) {
                LoadDefaults();
              } else {
                LoadEepromData(key);
                ChangeMainMenu(MAIN_MAP);
              }
              EditNumberMode = 0;
              break;
        case MAIN_SAVE:
              if (key == 0) {
                DeleteAllEepromData();
              } else {
                SaveEepromData(key);
                ChangeMainMenu(MAIN_MAP);
              }
              EditNumberMode = 0;
              break;
        case MAIN_OPTIONS:
                switch (EditMode) {
                  case 0:
                    if (key != 0){
                      EditModeId = constrain(key-1, 0, BLE_MAX-1);
                      EditNumberMode = 0;
                    }
                    break;
                  case 4:
                  case 5:
                  case 6:
                    EditNumber = constrain(key, 0, 1);
                    EditNumberMode = 0;
                    break;
                  default:
                    EditNumber = OverwriteNumber(EditNumber, key, EditNumberMode);
                    EditNumberMode = min(EditNumberMode+1, 2);
                }
                break;
         case MAIN_MAP:
                switch (EditMode) {
                  case 0: EditNumberMode = 0; break;
                  case 1:
                    if (key != 0){
                      EditModeId = constrain(key-1, 0, BLE_MAX-1);
                      EditNumberMode = 0;
                    }
                    break;
                  case 2:
                    if (key == 0) {
                      ControlMapReq++;
                      if ((ControlMapReq > 2) or (ControlMapAxisId == 255)) ControlMapReq = 0;
                      if (ControlMapReq == 0) {
                        ControlMapAxisId = 255;
                        ControlMapAxisAxis = 255;
                      }
                    } else {
                      EditModeChannel = constrain(key-1, 0, BLE_MAX-1);
                      ControlMapAxisId = SBrick[EditModeId].AxisId[EditModeChannel];
                      ControlMapAxisAxis = SBrick[EditModeId].AxisAxis[EditModeChannel];
                      ControlMapReq = SBrick[EditModeId].Req[EditModeChannel];
                      Sensor a;
                      a.x = 0;
                      a.y = 0;
                      a.button = false;
                      for (int j=0; j < 4; j++) {
                        if (j != ControlMapAxisId)                        TftPrintAnalogStickInfo(j, a, -1);
                      }
                      if (ControlMapAxisId < 255)                          TftPrintAnalogStickInfo(ControlMapAxisId, a, ControlMapAxisAxis);
                      EditNumberMode = 0;
                    }
                    break;
                  case 3:
                    if (key == 0) {
                      EditNumber++;
                      if (EditNumber > 2) EditNumber = 0;
                    }
                    break;
                  case 4:
                    EditNumber = OverwriteNumber(EditNumber, key, EditNumberMode);
                    EditNumberMode = min(EditNumberMode+1, 2);
                    break;
                }
                break;
        case MAIN_SWITCH:
            EditNumber = OverwriteNumber(EditNumber, key, EditNumberMode);
            EditNumberMode = min(EditNumberMode+1, 2);
          break;
        case MAIN_RUN:
        case MAIN_RUNTRAIN:
          switch (EditMode) {
            case 0:
              EditNumberMode = 0;
              EcecuteDirectControl(key);
              break;
            case 1:
              if (key != 0){
                EditModeId = constrain(key-1, 0, BLE_MAX-1);
                EditNumberMode = 0;
                LastControlledId = EditModeId;
                LastControlledChannel = 0;
              }
              break;
          }
          break;
      }
      Serial.println("KEYPAD number: " + String(key));
    }
  }
}
//*****************************************************************************************************************
// 
void EcecuteDirectControl(int key) {
  if ((LastControlledId >= 0) and (LastControlledChannel >= 0)) {
    if ((key == 0) and (SBrick[LastControlledId].PowerTarget[LastControlledChannel] == 0))
      SBrick[LastControlledId].Sign[LastControlledChannel] = !SBrick[LastControlledId].Sign[LastControlledChannel];
    int Power = map(key, 1, 8, SBrick[LastControlledId].Min, SBrick[LastControlledId].Max);
    int PowerRaw = map(Power, SBrick[LastControlledId].Min, SBrick[LastControlledId].Max, POWER_MIN_THR, POWER_MAX);
    if (key == 0) {
      Power = 0;
      PowerRaw = 0;
    }
    if (key == 9) {
      Power = 255;
      PowerRaw = 255;
    }
    //LogAdd("Power " + String(Power), TFT_GOLD);
    if (SBrick[LastControlledId].Sign[LastControlledChannel]) {
      Power = -Power;
      PowerRaw = -PowerRaw;
    }
    if (SBrick[LastControlledId].AxisId[LastControlledChannel] == 3) {
      RotaryTest(0, PowerRaw);
    } else {
      RequestPowerDirectW(Power, LastControlledId, LastControlledChannel);
      if (ControlMainMenu == MAIN_RUNTRAIN)
        RotarySet(0, PowerRaw);
    }
  }
}
//*****************************************************************************************************************
// 
bool FindNextChannel() {
  int id = max(LastControlledId, 0);
  int channel = max(LastControlledChannel, 0);
  for (int i=0; i < BLE_MAX+1; i++) {
    if (SBrick[id].Mode == SBRICK_MODE_CONNECTED_OK) {
      while (channel < BLE_CHANNEL_MAX-1) {
        channel++;
//      LogAdd("DEBUG loop: " + String(id+1) + ":" + String(channel) + "  " + String(SBrick[id].AxisId[channel]) + ":" + String(SBrick[id].Req[channel]), TFT_BLUE);
        if ((SBrick[id].AxisId[channel] < 255) and (SBrick[id].Req[channel] != DEVICE_REQ_IDLE)) {
          LastControlledId = id;
          LastControlledChannel = channel;
          return true;
        }
      }
      if (channel >= BLE_CHANNEL_MAX-1) channel = -1;
    }
    id++;
    if (id >= BLE_MAX) id = 0;
  }
  return false;
}
//*****************************************************************************************************************
// button callback function
void ButtonCallbackOnChange(int id, bool value) {
  //Serial.println("INPUT("+String(id)+")  button: " + String(value));
  if (value) {
    if ((ControlMainMenu == MAIN_RUNTRAIN) and (id == 3)) {
      if (FindNextChannel()) {
        EditModeId = LastControlledId;
        RotarySet(0, SBrick[LastControlledId].Power[LastControlledChannel]);
//      LogAdd("DEBUG reset: " + String(LastControlledId+1) + "/" + String(LastControlledChannel), TFT_BLUE);
      }
    } else {
      //auto mapping for STOP
      for (int i=0; i < BLE_MAX; i++) {
        for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
          if (SBrick[i].AxisId[channel] == id) RequestPowerDirect(0, i, channel);
        }
      }
    }
  }
}
//*****************************************************************************************************************
// axis callback function
void StickCallbackOnChange(int id, Sensor value) {
  int axis = -1;
  if ((ControlMainMenu == MAIN_MAP) and (EditMode == 2) and (ControlMapAxisId == id)) {
    axis = ControlMapAxisAxis;
  }
  TftPrintAnalogStickInfo(id, value, axis);
}
//*****************************************************************************************************************
// axis callback function
void AxisCallbackOnChange(int id, int axis, int value) {
  if (ControlMainMenu == MAIN_MAP) {
    if (value != 0 ) {
      ControlMapAxisId = id;
      ControlMapAxisAxis = ((value>0)<<1) + (axis & 0x01);
      if ((ControlMapReq == 0) or (id == 3)) ControlMapReq = 1;
      if ((id == 3) or (id == 4)) {
        ControlMapReq = 1;
        RotaryReset(id - 3);
      }
    }
  }
  if ((ControlMainMenu == MAIN_RUNTRAIN) and (id == 3) and (LastControlledId >= 0) and (LastControlledChannel >= 0)) {
    RequestPower(value, LastControlledId, LastControlledChannel);
  } else {
    //LogAdd("INPUT("+String(id)+") axis:" + String(axis) + ":" + String(value), TFT_WHITE);
    //auto mapping
    for (int i=0; i < BLE_MAX; i++)
      for (int channel=0; channel < BLE_CHANNEL_MAX; channel++)
        if (SBrick[i].AxisId[channel] == id)
          if ((SBrick[i].AxisAxis[channel] & 0x01) == axis) {
            bool invert = (SBrick[i].AxisAxis[channel] & 0x02) == 0 ;
            int power = value;
            if (invert) power = -value;
            uint8_t req = 1;
            if (MultiMode2) req = 2;
            if ((ControlMainMenu != MAIN_RUNMULTI) or ((ControlMainMenu == MAIN_RUNMULTI) and (SBrick[i].Req[channel] == req))) {
              RequestPower(power, i, channel);
              LastControlledId = i;
              LastControlledChannel = channel;
            }
          }
  }
}
//*****************************************************************************************************************
// arduino IDE setup entry point function
void setup() {
  Serial.begin(115200);
  TftInit();
  EepromInit();
  AnalogInit();
  AccelSensorInit();
  RotaryInit();
  KeypadInit();
  AxisCallback = &AxisCallbackOnChange;
  ButtonCallback = &ButtonCallbackOnChange;
  KeypadCallback = &KeypadCallbackOnChange;
  StickCallback = &StickCallbackOnChange;
  xTaskCreatePinnedToCore(initBLE, "BLE_init_task", 4096, NULL, 3, &BleHandle, 0);     //1332
  xTaskCreatePinnedToCore(initWifi, "WIFI_init_task", 4096, NULL, 3, &WifiHandle, 0);  //184
  pinMode(27, INPUT);
  int AkkuVoltageRaw = analogRead(27);
  AkkuVoltage = AkkuVoltageRaw * 0.00172;
  if (AkkuVoltage > 3) AkkuVoltageNoFirstRead = false;
}
//*****************************************************************************************************************
// arduino IDE loop entry point function
void loop() {
  static uint32_t VoltageTimestamp;
  WifiCyclicCall();
  if (OTAoff) {
    if (millis() > VoltageTimestamp){
      VoltageTimestamp = millis() + VOLTAGE_READINTERVAL;
      if (AkkuVoltageNoFirstRead)
      {
        int AkkuVoltageRaw = analogRead(39);
        AkkuVoltage = AkkuVoltageRaw * 0.00172;
      }
    }
    BLECyclicCall();
    TftCyclicCall();
    AnalogCyclicCall();
    AccelSensorCyclicCall();
    RotaryCyclicCall();
    KeypadCyclicCall();
  }
}
