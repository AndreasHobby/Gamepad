//*****************************************************************************************************************
// ** header incldes and installation hints **
//*****************************************************************************************************************
// ** BLE configuration **
#define BLE_MIN_DELAY             200 //100
#define BLE_REQUEST_ALWAYS        true
#define BLE_REQUEST_DELAY         0
#define BLE_VOLTAGE_DELAY         60000
#define BLE_REQUEST_CYCLIC        2000
#define BLE_POS_CALC_CYCLIC       100
#define BLE_POS_MAX               50
static BLEUUID    serviceUUID("4dc591b0-857c-41de-b5f1-15abda665b0c"); // SBrick UUID 
static BLEUUID quickdriveUUID("489a6ae0-c1ab-4c9c-bdb2-11d373c1b7fb"); // SBrick UUID quickdrive
static BLEUUID     remoteUUID("02b8cbcc-0e25-4bda-8790-a15f53e6010f"); // SBrick UUID remote
#define POS_THRESHOLD             0      //40
//#define POS_FACTOR                0.125  //0.0625 ///x*Gradient/256
//*****************************************************************************************************************
// ** BLE storage data **
BLEScan *pBLEScan;
uint32_t lastSBrickRequest = 0;
//*****************************************************************************************************************
//*****************************************************************************************************************
// convert function: power value from power in (range -255 .. 0 .. +255) into BLE device (MSB byte)
int FUN_ConvertPower(int power) {
  int p = abs(power)/2;
  if (power >= 0) {
    return p*2;
  } else {
    return p*2+1;
  }
}
//*****************************************************************************************************************
// 
bool updatePos(int *power, int input, int threshold) {
  int temp=*power;
  if ((input>threshold)or(input<-threshold)) {
    *power = *power + input;
  }
  *power = constrain(*power, -POWER_MAX, POWER_MAX);
  return *power != temp;
}
//*****************************************************************************************************************
// 
void updateData(int Power, int Index, int Channel) {
  int MinCheck = constrain(SBrick[Index].Min, 1, 8);
  if (Power > MinCheck) {
    SBrick[Index].PowerLim[Channel] = map(Power, MinCheck, 255, SBrick[Index].Min, SBrick[Index].Max);
  } else if (Power < -MinCheck) {
    SBrick[Index].PowerLim[Channel] = map(Power, -255, -MinCheck, -SBrick[Index].Max, -SBrick[Index].Min);
  } else {
    SBrick[Index].PowerLim[Channel] = 0;
  }
  if (abs(Power) <= POWER_MIN_THR) SBrick[Index].PowerLim[Channel] = 0;
  SBrick[Index].Channel[Channel] = FUN_ConvertPower(SBrick[Index].PowerLim[Channel]);
  SBrick[Index].UpdateRequest = true;
  SBrick[Index].offsetSBrickRequest = 0;
}
//*****************************************************************************************************************
// 
void RequestPowerDirect(int Power, int Index, int Channel) {
  if ((ControlMainMenu == MAIN_RUNTRAIN) or (SBrick[Index].AxisId[Channel] == 3)) RotarySet(0, Power);
  SBrick[Index].PowerTarget[Channel] = Power;
  SBrick[Index].Power[Channel] = Power;
  if (Power < 0)  SBrick[Index].Sign[Channel] = true;
  if (Power > 0)  SBrick[Index].Sign[Channel] = false;
  updateData(SBrick[Index].Power[Channel], Index, Channel);
}
//*****************************************************************************************************************
// 
void RequestPowerDirectW(int Power, int Index, int Channel) {
  //if ((ControlMainMenu == MAIN_RUNTRAIN) or (SBrick[Index].AxisId[Channel] == 3)) RotarySet(0, Power);
  SBrick[Index].PowerTarget[Channel] = Power;
  SBrick[Index].Power[Channel] = Power;
  SBrick[Index].PowerLim[Channel] = Power;
  if (Power < 0)  SBrick[Index].Sign[Channel] = true;
  if (Power > 0)  SBrick[Index].Sign[Channel] = false;
  if (abs(Power) <= POWER_MIN_THR) SBrick[Index].PowerLim[Channel] = 0;
  SBrick[Index].Channel[Channel] = FUN_ConvertPower(SBrick[Index].PowerLim[Channel]);
  SBrick[Index].UpdateRequest = true;
  SBrick[Index].offsetSBrickRequest = 0;
}
//*****************************************************************************************************************
// 
void RequestPower(int Power, int Index, int Channel) {
  SBrick[Index].PowerTarget[Channel] = Power;
  SBrick[Index].offsetCalcRequest = 0;
  SBrick[Index].UpdateReq[Channel] = true;
  if (Power < 0)  SBrick[Index].Sign[Channel] = true;
  if (Power > 0)  SBrick[Index].Sign[Channel] = false;
}
//*****************************************************************************************************************
// 
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }
  void onDisconnect(BLEClient* pclient) {
    String address = pclient->getPeerAddress().toString().c_str();
    int index = FindIndex(address);
    if (index >= 0) {
      LogAdd("BLE: (" + String(index) + ")" + DecodeAdress(address) + " discon", TFT_GOLD);
      SBrick[index].Type = SBRICK_DEVICE_HTTP;
      DataInit(index);
    }
  }
};
//*****************************************************************************************************************
// function to initialize data
void DataInit(int i) {
    SBrick[i].UpdateRequest = false;
    if (SBrick[i].Type == SBRICK_DEVICE_HTTP) {
      SBrick[i].Mode = SBRICK_MODE_CONNECTED;
    } else {
      SBrick[i].Mode = SBRICK_MODE_NEED_SCAN;
    }
    SBrick[i].RSSI = 0;
    SBrick[i].Voltage = 0;
    for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
      SBrick[i].Channel[channel] = 0;
      SBrick[i].Power[channel] = 0;
      SBrick[i].PowerTarget[channel] = 0;
      SBrick[i].UpdateReq[channel] = false;
    }
}
//*****************************************************************************************************************
// function to find already used addresses and return index
int FindIndex(String Address) {
  for (int i=0; i < BLE_MAX; i++) {
    if (Address == SBrick[i].Address){
      return i;
    }
  }
  return -1;
}
//*****************************************************************************************************************
// function to get new index
int GetNewIndex(String Address) {
  //first check for existing address
  int index = FindIndex(Address);
  if (index ==-1){
    //find first empty address
    for (int i=0; i < BLE_MAX; i++) {
      if (SBrick[i].Address == ""){
        index = i;
        break;
      }
    }
  }
  return index;
}
//*****************************************************************************************************************
// 
String DecodeAdress(String adress){
  String S = "";
  if (adress.length() > 0) S = adress.substring(0,2) + adress.substring(3,5) + adress.substring(6,8) + adress.substring(9,11) + adress.substring(12,14) + adress.substring(15,17);
  return S;
}
//*****************************************************************************************************************
// function to connect to SBrick
bool connectToSBrick(int id) {
  if (SBrick[id].Device == nullptr){
    return false;
  }
  SBrick[id].Mode = SBRICK_MODE_CONNECTING;
  //LogAdd("create client", TFT_WHITE);
  SBrick[id].pClient = BLEDevice::createClient();
  //LogAdd("set callbacks", TFT_WHITE);
  SBrick[id].pClient->setClientCallbacks(new MyClientCallback());
  //LogAdd("connect", TFT_WHITE);
  SBrick[id].pClient->connect(SBrick[id].Device);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  //LogAdd("get service", TFT_WHITE);
  BLERemoteService* pRemoteService = SBrick[id].pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)                                       {SBrick[id].pClient->disconnect();LogAdd("BLE: error: no service", TFT_RED);return false;}
  //LogAdd("get char quickdrive", TFT_WHITE);
  SBrick[id].pCharacteristic_QuickDrive = pRemoteService->getCharacteristic(quickdriveUUID);
  if (SBrick[id].pCharacteristic_QuickDrive == nullptr)                {SBrick[id].pClient->disconnect();LogAdd("BLE: error: no char", TFT_RED);return false;}
  //LogAdd("get char remote", TFT_WHITE);
  SBrick[id].pCharacteristic_RemoteControl = pRemoteService->getCharacteristic(remoteUUID);
  if (SBrick[id].pCharacteristic_RemoteControl == nullptr)             {SBrick[id].pClient->disconnect();LogAdd("BLE: error: no char", TFT_RED);return false;}
  if (SBrick[id].pCharacteristic_RemoteControl->canNotify() == false)  {LogAdd("BLE: error: no notify", TFT_RED);}
//  std::string value = SBrick[id].pCharacteristic_RemoteControl->readValue();
//  const uint8_t bothOn[] = {0x1, 0x0};
//  SBrick[id].pCharacteristic_RemoteControl->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)bothOn, 2, true);
//  SBrick[id].pCharacteristic_RemoteControl->registerForNotify(notifyCallback);
  DataInit(id);
  SBrick[id].Name = SBrick[id].Device->getName().c_str();
  SBrick[id].RSSI = SBrick[id].Device->getRSSI();
  SBrick[id].Type = SBRICK_DEVICE_BLE;
  return true;
}
//*****************************************************************************************************************
// 
void TakeOver() {
  for (int i=0; i < BLE_MAX; i++) {
    if ((ConnectAddress[i] != "") and (ConnectId[i] != 0)) {
      int index = ConnectId[i] - 1;
      //LogAdd("BLE: device (" + DecodeAdress(ConnectAddress[i]) + ") not in list", TFT_GOLD);
      SBrick[index].Address = ConnectAddress[i];
      SBrick[index].Device = ConnectDevice[i];
      if (connectToSBrick(index)) SBrick[index].Mode = SBRICK_MODE_CONNECTED_OK; else SBrick[index].Mode = SBRICK_MODE_CONNECTED;
      LogAdd("BLE: (" + String(index+1) + ") " + DecodeAdress(SBrick[index].Address), TFT_GREEN);
      vTaskDelay(100);
    }
  }
}
//*****************************************************************************************************************
// Scan function
bool Scan(){
  bool Found = false;
  pBLEScan = BLEDevice::getScan();
//  pBLEScan->setInterval(189);
//  pBLEScan->setWindow(29);
  pBLEScan->setActiveScan(true);
  BLEScanResults results =  pBLEScan->start(5);
  LogAdd("BLE: " + String(results.getCount()) + " devices found", TFT_GREEN);
  ConnectCount = 0;
  for(int i=0;i<results.getCount();i++) {
    String Name = results.getDevice(i).getName().c_str();
    String Address = results.getDevice(i).getAddress().toString().c_str();
    int rssi = results.getDevice(i).getRSSI();
    LogAdd(DecodeAdress(Address) + "/" + String(rssi) + "/" + Name, TFT_ORANGE);
    if ((Name == "SBrick") or (Name == "SBrick ")) {          //only connect to knwon devices via Name
      int index = FindIndex(Address);
      if (index >= 0) {
        SBrick[index].Address = Address;
        SBrick[index].Device = new BLEAdvertisedDevice(results.getDevice(i));
        Found = connectToSBrick(index);
        if (Found) SBrick[index].Mode = SBRICK_MODE_CONNECTED_OK; else SBrick[index].Mode = SBRICK_MODE_CONNECTED;
        LogAdd("BLE: (" + String(index+1) + ") " + DecodeAdress(SBrick[index].Address), TFT_GREEN);
      }
      else
      {
        Found = true;
        ConnectAddress[ConnectCount] = Address;
        ConnectDevice[ConnectCount] = new BLEAdvertisedDevice(results.getDevice(i));
        ConnectId[ConnectCount] = 0;
        ConnectCount++;
      }
    }
    vTaskDelay(100);
  }
  if (ConnectCount > 0) {
    int freeid = 0;
    for(int i=0;i<BLE_MAX;i++) {
      if (SBrick[i].Type != SBRICK_DEVICE_BLE) {
        ConnectId[min(freeid, BLE_MAX)] = i+1;
        freeid++;
      }
    }
    ChangeMainMenu(MAIN_NEWDEVICE);
    EditMode = 1;
  }
  return Found;
}
//*****************************************************************************************************************
// now scanning only at ESP32 startup
void initBLE(void*) {
  BLEDevice::init("esp32");
  while(1) {
    if (BLE_RequestScan) {
      LogAdd("BLE: scan starting...", TFT_WHITE);
      if (!Scan()) {
        vTaskDelay(100);
        //LogAdd("BLE: scan second run");
        Scan();
      }
      LogAdd("BLE: scan done", TFT_GREEN);
      BLE_RequestScan = false;
    }
    if (BLE_Takeover) {
      TakeOver();
      BLE_Takeover = false;
    }
    vTaskDelay(1000);
  }
}
//*****************************************************************************************************************
// BLE polling to request data from SBricks or derivates
int BLEGetConfigCount(int index) {
  int result = 0; 
  for (int channel=0; channel < BLE_CHANNEL_MAX; channel++)
    if (SBrick[index].Req[channel] != 0) result++;
  return result;
}
//*****************************************************************************************************************
// BLE polling to request data from SBricks or derivates
void BLECyclicCall() {
static int x;
  x++;
  if (x >= BLE_MAX) {
    x = 0;
  }
  uint32_t time = millis();
  if (time > SBrick[x].lastCalcRequest + SBrick[x].offsetCalcRequest) {
    SBrick[x].offsetCalcRequest = BLE_POS_CALC_CYCLIC;
    SBrick[x].lastCalcRequest = time;
    for (int channel=0; channel < BLE_CHANNEL_MAX; channel++) {
      if (SBrick[x].UpdateReq[channel]) {
        uint8_t req= SBrick[x].Req[channel];
        if ((ControlMainMenu == MAIN_MAP) or (ControlMainMenu == MAIN_RUNMULTI)) req = DEVICE_REQ_TARGET; //override postition mode !
        switch (req) {
          case DEVICE_REQ_TARGET:
          {
            if (SBrick[x].PowerTarget[channel] != SBrick[x].Power[channel]) {
              //LogAdd("BLE: smooth update" + String(channel) + ":" + String(SBrick[x].PowerTarget[channel]) + " Power:" + String(SBrick[x].Power[channel]));
              SBrick[x].Power[channel] = SBrick[x].Power[channel] + constrain(SBrick[x].PowerTarget[channel] - SBrick[x].Power[channel], -SBrick[x].Gradient, SBrick[x].Gradient);
              SBrick[x].Power[channel] = constrain(SBrick[x].Power[channel], -POWER_MAX, POWER_MAX);
              updateData(SBrick[x].Power[channel], x, channel);
            } else {
//            LogAdd("BLE: smooth reset" + String(channel) + ":" + String(SBrick[x].PowerTarget[channel]) + " Power:" + String(SBrick[x].Power[channel]));
              SBrick[x].UpdateReq[channel] = false;
            }
          }
          break;
         case DEVICE_REQ_POSITION:
          {
            if (updatePos(&SBrick[x].Power[channel], SBrick[x].PowerTarget[channel]*SBrick[x].Gradient/256*MainPositionGradient/256, POS_THRESHOLD)) {
              //LogAdd("BLE: pos update" + String(channel) + ":" + String(SBrick[x].PosTarget[channel]) + " Power:" + String(SBrick[x].Power[channel]));
              updateData(SBrick[x].Power[channel], x, channel);
            } else {
//            LogAdd("BLE: pos reset" + String(channel) + ":" + String(SBrick[x].PosTarget[channel]) + " Power:" + String(SBrick[x].Power[channel]));
              SBrick[x].UpdateReq[channel] = false;
            }
          }
          break;
        default:
          break;
        }
      }
    }
  }
  
  if ((time > SBrick[x].lastSBrickRequest + SBrick[x].offsetSBrickRequest) and (time > lastSBrickRequest + BLE_MIN_DELAY)) {
    bool PowerRequested = ((SBrick[x].Channel[0]!=0) or (SBrick[x].Channel[1]!=0) or (SBrick[x].Channel[2]!=0) or (SBrick[x].Channel[3]!=0));
    if ((SBrick[x].Mode >= SBRICK_MODE_CONNECTED_FAIL) and (BLEGetConfigCount(x) > 0)) {
      //LogAdd("connected:"+String(SBrick[0].Mode)+"/"+String(SBrick[1].Mode)+"/"+String(SBrick[2].Mode)+"/"+String(SBrick[3].Mode));
      bool sendData = (SBrick[x].UpdateRequest) or (PowerRequested);
      SBrick[x].UpdateRequest = false;
      SBrick[x].offsetSBrickRequest = BLE_REQUEST_CYCLIC;
      lastSBrickRequest = time;
      SBrick[x].lastSBrickRequest = lastSBrickRequest;
      if (SBrick[x].Type == SBRICK_DEVICE_BLE) {
        if ((sendData == false) and (millis() > SBrick[x].lastVoltageRequest)) {
          SBrick[x].lastVoltageRequest = millis() + BLE_VOLTAGE_DELAY;
          uint8_t req[2];
          req[0] = 0x0F;
          req[1] = 0x08;
          SBrick[x].pCharacteristic_RemoteControl->writeValue(req, 2, true);
          delay(3);
          uint16_t value = SBrick[x].pCharacteristic_RemoteControl->readUInt16();
          value = (value & 0xFF00) >> 4;
          value = value*0.066;  // 10*0.83875/127;
          SBrick[x].Voltage = value*0.1;
          SBrick[x].RSSI = SBrick[x].Device->getRSSI();
        } else
        if (sendData or BLE_REQUEST_ALWAYS) {
          SBrick[x].pCharacteristic_QuickDrive->writeValue(SBrick[x].Channel, BLE_CHANNEL_MAX);
          if (sendData) {
//          LogAdd("BLE: req:("+String(x) + "): " + String(SBrick[x].PowerLim[0])+" "+String(SBrick[x].PowerLim[1])+" "+String(SBrick[x].PowerLim[2])+" "+String(SBrick[x].PowerLim[3]), TFT_GREEN);
          }
        }
      }
      if (SBrick[x].Type == SBRICK_DEVICE_HTTP) {
        if (sendData) {
          int color;
          if (sendHtmlPower(x) > 0) {
            SBrick[x].Mode = SBRICK_MODE_CONNECTED_OK;
            color = TFT_GREEN;
          } else {
            SBrick[x].Mode = SBRICK_MODE_CONNECTED_FAIL;
            color = TFT_RED;
          }
//        LogAdd("HTML: req:("+String(x) + "): " + String(SBrick[x].PowerLim[0]), color);
        }
      }
    }
  }
}
