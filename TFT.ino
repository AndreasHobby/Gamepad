//*****************************************************************************************************************
// ** header incldes and installation hints **
//*****************************************************************************************************************
#include <WiFi.h>                //built in via ESP32 installation
#include <SPI.h>
#include "switch.h"
//*****************************************************************************************************************
// ** TFT configuration **
#define TFT_UPDATETIME    200
#define TFT_BLINKTIME     500
const String MenuText[12] = {"LOG", "MAP", "TRACK", "RUN", "INFO", "OPTIONS", "RUN ZUG", "RUN MULT", "LOAD", "SAVE", "DEFINE ID", "ZUG LIST"};
//*****************************************************************************************************************
// ** TFT storage data **
TFT_eSPI tft = TFT_eSPI();       // Invoke library, pins defined in User_Setup.h
uint32_t TftLastUpdate = 0;
uint32_t TftLastBlink = 0;
bool TftBlink = false;
//*****************************************************************************************************************
//*****************************************************************************************************************
// 
void TftInit() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  TftDrawMenu(ControlMainMenu);
  LogAdd("DISPLAY: init done", TFT_GREEN);
}
void TftFlashStart() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString("FLASHING", 20, 48, 4);
}
void TftFlashProgress(unsigned int progress, unsigned int total) {
  int percent = progress*100/total;
  tft.drawString(String(percent) + "%", 70, 90, 2);
}
//*****************************************************************************************************************
// 
void TftDrawMenu(int menuitem) {
  tft.fillRect(0, 0, 64, 16, TFT_BLACK);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString(MenuText[menuitem], 1, 0, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawLine(0,16,159,16, TFT_WHITE);
  tft.drawLine(0,100,159,100, TFT_WHITE);
  tft.drawLine(0,119,159,119, TFT_WHITE);
  
  if (ControlMainMenu == MAIN_SWITCH){
    tft.fillRect(0, 17,160, 102, TFT_BLACK);
    tft.drawXBitmap(0, 17, pic_track, pic_width, pic_height, TFT_WHITE);
    tft.drawXBitmap(0, 17, pic_text, pic_width, pic_height, TFT_GOLD);
  }
}
//*****************************************************************************************************************
// 
void TftColorText(bool mode, int color) {
  if (mode) {
    tft.setTextColor(color, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_BLACK, TFT_BLACK);    
  }
}
//*****************************************************************************************************************
// 
void TftColorBlink(int color) {
  if (TftBlink) {
    tft.setTextColor(color, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_BLACK, color);    
  }
}
//*****************************************************************************************************************
// 
bool BLEconnected(){
  bool BLEconnected = false;
  for (int i=0; i < BLE_MAX; i++)
    if ((SBrick[i].Type == SBRICK_DEVICE_BLE) and (SBrick[i].Mode > SBRICK_MODE_CONNECTED_FAIL))
      BLEconnected = true;
  return BLEconnected;
}
//*****************************************************************************************************************
// 
void TftDrawChannel() {
  int y = 19;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (ControlMainMenu != MAIN_NEWDEVICE) tft.drawString("Id", 0, y, 1);
  int imax = BLE_MAX;
  if (ControlMainMenu == MAIN_NEWDEVICE) imax = ConnectCount;
  for (int i=0; i < imax; i++) {
    int y = 19 + (i+1)*9;
    int color;
    if (SBrick[i].Mode == SBRICK_MODE_CONNECTED_OK) {
      color = TFT_GREEN;
    } else if (SBrick[i].Mode == SBRICK_MODE_CONNECTED) {
      color = TFT_GOLD;
    } else if (SBrick[i].Mode == SBRICK_MODE_CONNECTED_FAIL) {
      color = TFT_RED;
    } else {
      color = TFT_WHITE;
    }
    if (ControlMainMenu == MAIN_NEWDEVICE) color = TFT_WHITE;
    tft.setTextColor(color, TFT_BLACK);
    if ((EditMode > 1) and (i == EditModeId))  tft.setTextColor(TFT_BLACK, color);
    if ((ControlMainMenu == MAIN_NEWDEVICE) and (EditMode == 0) and (i == EditModeId)) TftColorBlink(color);
    if ((ControlMainMenu == MAIN_MAP) and (EditMode == 1) and (i == EditModeId)) TftColorBlink(color);
    if ((ControlMainMenu == MAIN_RUNTRAIN) and (EditMode == 1) and (i == EditModeId)) TftColorBlink(color);
    if ((ControlMainMenu == MAIN_RUN) and (EditMode == 1) and (i == EditModeId)) TftColorBlink(color);
    tft.drawString(String(i+1), 0, y, 1);
  }
}
//*****************************************************************************************************************
// 
void TftCyclicCall() {
  static int EditModeOld;
  static int ControlMainMenuOld;
  uint32_t time = millis();
  if (time > TftLastBlink){
    TftLastBlink = time + TFT_BLINKTIME;
    TftBlink = !TftBlink;
  }
  if (time > TftLastUpdate){
    TftLastUpdate = time + TFT_UPDATETIME;
    
    bool RefreshReq = false;
    if (EditModeOld != EditMode)               RefreshReq = true;
    if (ControlMainMenuOld != ControlMainMenu) RefreshReq = true;
    if (TftLogUpdateRequest) {
      tft.fillRect(0, 120,160, 8, TFT_BLACK);
      tft.setTextColor(LogColor[LOG_MAX-1], TFT_BLACK);
      tft.drawString(LogText[LOG_MAX-1], 0, 121, 1);
      TftLogUpdateRequest = false;
      if (ControlMainMenu == 0) RefreshReq = true;
    }
    if ((RefreshReq) and (ControlMainMenu != MAIN_SWITCH)) {
      tft.fillRect(0, 17, 160, 83, TFT_BLACK);
      tft.fillRect(0, 101,160, 18, TFT_BLACK);
    }

    int ble_color = TFT_BLACK;
    if ((BLEconnected()) or ((BLE_RequestScan) and (TftBlink))) ble_color = TFT_CYAN;
    tft.drawXBitmap(128, 0, pic_ble, ble_width, ble_height, ble_color);

    int wifi_color = TFT_BLACK;
    if (WiFi.status() == WL_CONNECTED) wifi_color = TFT_GOLD;
    if ((HTTP_RequestScan) and (TftBlink)) wifi_color = TFT_RED;
    tft.drawXBitmap(144, 0, pic_wifi, wifi_width, wifi_height, wifi_color);

    if (ControlMainMenu == MAIN_TRAININFO) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_SAVE], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString("Lokzuordnung", 0, 19 + 0*9, 1);
      for (int id=0; id < NAME_MAX; id++) {
        int x = 0;
        int y = 19 + (1+id)*9;
        if (id > 7) {
          x = 80;
          y = y = 19 + (1+id-8)*9;
        }
        tft.drawString(String(120+id), x, y, 1);
        tft.drawString(NameList[id], x+18, y, 1);
      }
    }
    
    if (ControlMainMenu == MAIN_NEWDEVICE) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString(String(EditModeId), 0, 111, 1);
      tft.drawString(String(EditMode), 50, 111, 1);
      tft.drawString(String(ConnectCount), 100, 111, 1);
      TftDrawChannel();
      for (int i=0; i < BLE_MAX; i++) {
        int y = 19 + (i+1)*9;
        if (ConnectAddress[i] != "") {
          if ((EditMode == 1) and (i == EditModeId)) TftColorBlink(TFT_WHITE); else tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.drawString(String(ConnectId[i]), 15, y);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.drawString(ConnectAddress[i], 40, y, 1);
        }
      }
    }
    if (ControlMainMenu == MAIN_LOG) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_INFO], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString("* STOP ALL", 0, 111, 1);
      for (int i=0; i < LOG_MAX_SHOW; i++) {
        tft.setTextColor(LogColor[i+LogPosition], TFT_BLACK);
        tft.drawString(LogText[i+LogPosition], 0, 19 + i*9, 1);
      }
    }
    if (ControlMainMenu == MAIN_INFO) {
      TftDrawChannel();
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString("6 SCAN", 0, 111, 1);
      tft.drawString("7 SCAN", 40, 111, 1);
      tft.drawString("8 ZUG INFO", 80, 111, 1);
      tft.drawString("Volt", 14, 19, 1);
      tft.drawString("RSSI", 40, 19, 1);
      tft.drawString("Adresse", 80, 19, 1);
      char voltage[5];
      for (int i=0; i < BLE_MAX; i++) {
        int y = 19 + (i+1)*9;
        if (SBrick[i].Voltage > 1) {
          dtostrf(SBrick[i].Voltage, 5, 1, voltage );
          
          {  int color = TFT_GOLD;
            if (SBrick[i].Voltage > 4.02) color = TFT_GREEN;
            if (SBrick[i].Voltage < 3.92) color = TFT_RED;
            tft.setTextColor(color, TFT_BLACK);
          }
          tft.drawString(voltage, 8, y, 1);
        }
        int rssi = SBrick[i].RSSI;
        if (rssi != 0) {
          int color = TFT_GOLD;
          if (rssi > -60) color = TFT_GREEN;
          if (rssi < -75) color = TFT_RED;
          tft.setTextColor(color, TFT_BLACK);
          tft.drawString(String(rssi), 40, y, 1);
        } else {
          tft.drawString("-", 40, y, 1);
        }
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (SBrick[i].Type == SBRICK_DEVICE_BLE) {
          tft.drawString(DecodeAdress(SBrick[i].Address), 65, y, 1);
        }
        if (SBrick[i].Type == SBRICK_DEVICE_HTTP) {
          if (SBrick[i].IP >= 120) {
            tft.drawString(NameList[min(SBrick[i].IP - 120, NAME_MAX-1)], 65, y, 1);
          }
          //tft.drawString(PreIP + StringFormat(MainIP, 3)+".", 65, y, 1);
          tft.drawString(".." + StringFormat(MainIP, 3)+".", 102, y, 1);
          tft.drawString(StringFormat(SBrick[i].IP, 3), 140, y, 1);
        }
      }
    }
    if (ControlMainMenu == MAIN_OPTIONS) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      if (EditMode == 0) {
        tft.drawString("# EDIT", 0, 111, 1);
      } if (EditMode == 1) {
        tft.drawString("# OK", 0, 111, 1);
        tft.drawString("* CANCEL", 50, 111, 1);
        tft.drawString("1-8 CHANNEL", 100, 111, 1);
      } else {
        tft.drawString("# OK", 0, 111, 1);
        tft.drawString("* CANCEL", 50, 111, 1);
        tft.drawString("0-9 DATA", 100, 111, 1);
      }
      tft.drawString("OPTION:", 0, 19, 1);
      tft.drawString("VALUE:", 80, 19, 1);
      tft.drawString("IP:", 0, 19+1*9, 1);
      if (EditMode == 1) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 3), 80, 19+1*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(StringFormat(MainIP, 3), 80, 19+1*9, 1);
      }
      
      tft.drawString(".", 100, 19+1*9, 1);
      tft.drawString(String(WiFi.localIP()[3]), 108, 19+1*9, 1);

      tft.drawString("IP Switch:", 0, 19+2*9, 1);
      if (EditMode == 2) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 3), 80, 19+2*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(StringFormat(MainSwitchIP, 3), 80, 19+2*9, 1);
      }
      tft.drawString("PositionGrad:", 0, 19+3*9, 1);
      if (EditMode == 3) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 3), 80, 19+3*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(StringFormat(MainPositionGradient, 3), 80, 19+3*9, 1);
      }
      tft.drawString("WIFI:", 0, 19+4*9, 1);
      if (EditMode == 4) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 1), 80, 19+4*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(String(MainWifi), 80, 19+4*9, 1);
      }
      tft.drawString("BLE:", 0, 19+5*9, 1);
      if (EditMode == 5) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 1), 80, 19+5*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(String(MainBle), 80, 19+5*9, 1);
      }
      tft.drawString("Accel:", 0, 19+6*9, 1);
      if (EditMode == 6) {
        TftColorBlink(TFT_WHITE);
        tft.drawString(StringFormat(EditNumber, 1), 80, 19+6*9, 1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      } else {
        tft.drawString(String(MainAccel), 80, 19+6*9, 1);
      }
      tft.drawString("RSSI:", 100, 19+4*9, 1);
      int rssi = WiFi.RSSI();
      if (rssi != 0) {
        int color = TFT_GOLD;
        if (rssi > -60) color = TFT_GREEN;
        if (rssi < -75) color = TFT_RED;
        tft.setTextColor(color, TFT_BLACK);
        tft.drawString(String(rssi), 140, 19+4*9, 1);
      } else {
        tft.drawString("-", 140, 19+4*9, 1);
      }
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("Voltage:", 0, 19+7*9, 1);
      {  int color = TFT_GOLD;
        if (AkkuVoltage > 4.02) color = TFT_GREEN;
        if (AkkuVoltage < 3.92) color = TFT_RED;
        tft.setTextColor(color, TFT_BLACK);
      }
      tft.drawString(String(AkkuVoltage) + "V", 80, 19+7*9, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      
      tft.drawString(String(compile_date), 20, 19+8*9, 1);
    }
    if (ControlMainMenu == MAIN_MAP) {
      TftDrawChannel();
      int y = 19;
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_LOAD], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      switch (EditMode) {
        case 0:
          tft.drawString("# EDIT", 0, 111, 1);
          tft.drawString("* STOP ALL", 50, 111, 1);
          break;
        default:
          tft.drawString("# OK", 0, 111, 1);
          tft.drawString("* BACK/STOP", 50, 111, 1);
          if (EditMode == 1) tft.drawString("1-8 ID", 100, 111, 1);
          if (EditMode == 2) tft.drawString("0-4 CHAN", 100, 111, 1);
      }
      tft.drawString("Cfg", 110, y, 1);
      tft.drawString("IP", 140, y, 1);
      for (int i=0; i < BLE_MAX; i++) {
        int y = 19 + (i+1)*9;
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
          if ((EditMode == 2) and (i == EditModeId) and (channel == EditModeChannel)){
            TftColorBlink(TFT_WHITE);
          } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);  
          }
          int id, axis, req;
          String S = "";
          if ((EditMode == 2) and (i == EditModeId) and (channel == EditModeChannel)){
            id = ControlMapAxisId;
            axis = ControlMapAxisAxis;
            S = String(ControlMapReq);
            Sensor a;
            a.x = 0;
            a.y = 0;
            a.button = false;
            for (int j=0; j < 4; j++) {
              if (j != ControlMapAxisId) TftPrintAnalogStickInfo(j, a, -1);
            }
          } else {
            id = SBrick[i].AxisId[channel];
            axis = SBrick[i].AxisAxis[channel];
            S = String(SBrick[i].Req[channel]);
          }
          if (S == "0") S = "";
          switch (axis){
            case 0:  S = "y" + S; break;
            case 1:  S = "x" + S; break;
            case 2:  S = "Y" + S; break;
            case 3:  S = "X" + S; break;
            default: S = " " + S; break;
          }
          switch (id){
            case 0:  S = "A" + S; break;
            case 1:  S = "L" + S; break;
            case 2:  S = "R" + S; break;
            case 3:  S = "M" + S; break;
            default: S = " " + S; break;
          }
          tft.drawString(S, 15+channel*25, y, 1);
          tft.drawString("Ch" + String(channel+1), 15+channel*25, 19, 1);
        }
        int Char;
        if ((EditMode == 3) and (i == EditModeId)){
          TftColorBlink(TFT_WHITE);
          Char = EditNumber;
        } else {
          tft.setTextColor(TFT_WHITE, TFT_BLACK);  
          Char = SBrick[i].Char;
        }
        tft.setCursor(115, y, 1);
        switch (Char){
          case 0: tft.print("L"); break;
          case 1: tft.print("S"); break;
          case 2: tft.print("M"); break;
        }

        if (SBrick[i].Type == SBRICK_DEVICE_HTTP) {
          if ((EditMode == 4) and (i == EditModeId)){
            TftColorBlink(TFT_WHITE);
            tft.drawString(StringFormat(EditNumber, 3), 130, y, 1);
          } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);  
            tft.drawString(StringFormat(SBrick[i].IP, 3), 130, y, 1);
          }
        }
      }
    }
    if (ControlMainMenu == MAIN_LOAD) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_SAVE], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString("1-9 LOAD", 0, 111, 1);
      tft.drawString("0 LOAD DEFAULT", 80, 111, 1);
    }
    if (ControlMainMenu == MAIN_SAVE) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D " +  MenuText[MAIN_RUN], 128, 102, 1);
      tft.drawString("1-9 SAVE", 0, 111, 1);
      tft.drawString("0 DELETE ALL", 80, 111, 1);
    }
    if ((ControlMainMenu == MAIN_LOAD) or (ControlMainMenu == MAIN_SAVE)) {
      for (int i=0; i < LOAD_MAX; i++) {
        int y = 19 + i*9;
        tft.drawString(String(i+1)+ " " + LoadText[i], 10, y, 1);
      }
    }
    if (ControlMainMenu == MAIN_SWITCH) {
      TftColorBlink(TFT_WHITE);
      tft.drawString(StringFormat(EditNumber, 2), 1, 111, 1);
    }
    if ((ControlMainMenu == MAIN_RUN) or (ControlMainMenu == MAIN_RUNTRAIN) or (ControlMainMenu == MAIN_RUNMULTI)) {
      TftDrawChannel();
      int y = 19;
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString("A " +  MenuText[MAIN_LOG], 0, 102, 1);
      tft.drawString("B " +  MenuText[MAIN_MAP], 40, 102, 1);
      tft.drawString("C " +  MenuText[MAIN_SWITCH], 80, 102, 1);
      tft.drawString("D MODE", 128, 102, 1);
      switch (EditMode) {
        case 0:
          tft.drawString("0 STOP # +-", 0, 111, 1);
          tft.drawString("0-9 CONTROL", 80, 111, 1);
          break;
        case 1:
          tft.drawString("0 STOP", 0, 111, 1);
          tft.drawString("1-8 CHANNEL", 80, 111, 1);
          break;
        case 2:
          tft.drawString("0 STOP", 0, 111, 1);
          tft.drawString("M STEP", 80, 111, 1);
          break;
      }
      tft.drawString("Power", 20, y, 1);
      for (int id=0; id < BLE_MAX; id++) {
        int y = 19 + (1+id)*9;
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if ((SBrick[id].IP >= 120) and (SBrick[id].Type == SBRICK_DEVICE_HTTP))
          tft.drawString(NameList[SBrick[id].IP - 120], 126, y, 1);
        for (int channel=0; channel < BLE_CHANNEL_MAX; channel++)
          TftUpdatePower(id, channel, (ControlMainMenu == MAIN_RUNTRAIN) and (id == LastControlledId) and (channel == LastControlledChannel) );
      }
      
      if (ControlMainMenu == MAIN_RUNMULTI) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        if ( MultiMode2) tft.drawString("2", 150, 111, 1);
        if (!MultiMode2) tft.drawString("1", 150, 111, 1);
      }
    }
    ControlMainMenuOld = ControlMainMenu;
    EditModeOld = EditMode;
  }
}
//*****************************************************************************************************************
// 
void TftUpdatePower(int id, int channel, bool active) {
  if ((ControlMainMenu == MAIN_RUN) or (ControlMainMenu == MAIN_RUNTRAIN) or (ControlMainMenu == MAIN_RUNMULTI)) {
    if ((SBrick[id].AxisId[channel] < 255) and (SBrick[id].Req[channel] != DEVICE_REQ_IDLE)) {
      int startx = 10;
      int factor = 32;
      int r = 2;
      int dy = 9;
      int dx = (256 / factor);
      int cx = 4*dx+5; //4
      int color = TFT_WHITE;
      if (active and TftBlink)
        color = TFT_GREEN;
      int starty = 19 + (id+1)*9;
      int x = startx+channel*cx + dx + SBrick[id].PowerLim[channel]/factor;
      int percent = (SBrick[id].PowerLim[channel])/2.56;
      if (percent == 0) {
        if (SBrick[id].PowerLim[channel] > POWER_MIN_THR) percent = 1;
        if (SBrick[id].PowerLim[channel] < -POWER_MIN_THR) percent = -1;
      }
      tft.setTextColor(color, TFT_BLACK);
      tft.fillRect(startx + channel * cx - 1, starty, cx+1, dy, TFT_BLACK);
      String sign = "";
      if (active and SBrick[id].Sign[channel] and (percent == 0))
        sign = "-";
      tft.drawString(sign + String(percent), startx + 2*dx + channel*cx + 2, starty, 1);
      tft.fillRect(x - 1, starty+1, r, dy - 2, TFT_RED);
      tft.drawRect(startx + channel * cx - 1, starty, 2*dx + 2, dy, color);
    }
  }
}
//*****************************************************************************************************************
// 
void TftPrintAnalogStickInfo(int id, Sensor value, int axis) {
  if (id < 0) return;
//  if (id > 2) return;
  int startx;
  int starty = 8;
  switch (id) {
    case 0: startx =  73; break;
    case 1: startx =  91; break;
    case 2: startx = 119; break;
    case 3: startx = 105; break;
  }
  int color;
  int r = 2;       //3
  int factor = 32; //32
  int dy = (256 / factor);
  int dx = dy;
  int x = startx - value.x / factor;
  int y = starty - value.y / factor;
  if (id >= 3) {
    dx = dx / 2;
  }
  tft.fillRect(startx-dx-1, starty-dy-1, 2*dx+3, 2*dy+3, TFT_BLACK);
  if (value.button) {
    color = TFT_RED;
  } else {
    color = TFT_WHITE;
  }
  switch (axis) {
    case 0: tft.fillRect(startx, starty-1, dx, r, TFT_GOLD); break;
    case 1: tft.fillRect(startx-1, starty, r, dy, TFT_GOLD); break;
    case 2: tft.fillRect(startx-dx, starty-1, dx, r, TFT_GOLD); break;
    case 3: tft.fillRect(startx-1, starty-dy, r, dy, TFT_GOLD); break;
  }
  tft.fillEllipse(x, y, r, r, color);
  tft.drawRoundRect(startx-dx, starty-dy, 2*dx+1, 2*dy+1, 3, TFT_WHITE); //5
  tft.drawLine(0,16,159,16, TFT_WHITE);
}
