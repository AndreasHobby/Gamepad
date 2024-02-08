//*****************************************************************************************************************
// ** header incldes and installation hints **
#include <BLEDevice.h>                     //url=https://github.com/nkolban/ESP32_BLE_Arduino (or built in via ESP32 in newer installations)
#include <TFT_eSPI.h>            // Graphics and font library for ST7735 driver chip
//*****************************************************************************************************************
// ** configuration **
#define TFT_GREY 0x5AEB // New colour
// SBrick controller mode
#define SBRICK_MODE_NEED_SCAN      0
#define SBRICK_MODE_NOT_CONNECTED  1
#define SBRICK_MODE_CONNECTING     2
#define SBRICK_MODE_CONNECTED_FAIL 3
#define SBRICK_MODE_CONNECTED      4
#define SBRICK_MODE_CONNECTED_OK   5
#define SBRICK_DEVICE_NONE         0
#define SBRICK_DEVICE_HTTP         1
#define SBRICK_DEVICE_BLE          2
#define DEVICE_REQ_IDLE            0
#define DEVICE_REQ_TARGET          1
#define DEVICE_REQ_POSITION        2
#define DEVICE_CHAR_LOC            0
#define DEVICE_CHAR_SMOOTH         1
#define DEVICE_CHAR_MAX            2

#define BLE_MAX                    8
#define LOG_MAX_SHOW               9
#define LOG_MAX                    30 //18
#define BLE_CHANNEL_MAX            4
#define POWER_MAX                  255
#define POWER_MIN                  1
#define POWER_MIN_THR              5
#define AXIS_MAX                   4
#define VOLTAGE_READINTERVAL       10000

#define MAIN_LOG                   0
#define MAIN_MAP                   1
#define MAIN_SWITCH                2
#define MAIN_RUN                   3
#define MAIN_INFO                  4
#define MAIN_OPTIONS               5
#define MAIN_RUNTRAIN              6
#define MAIN_RUNMULTI              7
#define MAIN_LOAD                  8
#define MAIN_SAVE                  9
#define MAIN_NEWDEVICE            10
#define MAIN_TRAININFO            11
const String PreIP = "192.168.";
const char keys[] = "123A456B789C*0#DNF";  // N = Nokey, F = Fail
const char compile_date[] = __DATE__ " " __TIME__;
//*****************************************************************************************************************
// ** main storage data **
int OTAoff = true;
String IP = "";                            //stores current IP address
int LogPosition = LOG_MAX - LOG_MAX_SHOW;
bool HTTP_RequestScan = false;
int HTTP_RequestScanIP = 120;
int HTTP_RequestScanFound = 0;
bool BLE_RequestScan = false;
bool BLE_Takeover = false;
int ControlMainMenu = MAIN_LOG;
int ControlMainMenuLast = MAIN_LOG;
int EditMode = 0;
int EditModeId = 0;
int EditNumberMode = 0;
int EditNumber = 0;
int EditModeChannel = 0;
int EditAxisId = 0;
int EditAxisAxis = 0;
uint8_t MainIP;
uint8_t MainSwitchIP;
uint8_t MainWifi;
uint8_t MainBle;
uint8_t MainAccel;
uint8_t MainPositionGradient;
int LastControlledId = -1;
int LastControlledChannel = -1;
bool MultiMode2 = false;
int ControlMapAxisId = 255;
int ControlMapAxisAxis = 255;
int ControlMapReq = 1;
String LogText[LOG_MAX];
int LogColor[LOG_MAX];
bool TftLogUpdateRequest = false;
bool SwitchState[256];
bool AkkuVoltageNoFirstRead = true;
float AkkuVoltage = 0;
//                      LOK  SMOOTH  MAX
const int PreGrad[3] = { 40,   40,  255};
const int PreMin[3]  = {150,   90,    1};
const int PreMax[3]  = {200,  255,  255};

#define NAME_MAX  16
const String NameList[NAME_MAX]  = {"Dreh", "BR01", "BR23", "BR103", "BR146", "V200", "V36", "BR218", "Kroko", "BR151", "H3gelb", "BR212", "BR212", "BR423", "H3weis", "BR65"};
#define LOAD_MAX  9
const String LoadText[LOAD_MAX]  = {"Zug #1", "Zug #2", "", "", "", "", "Auto #1", "Auto #2", "Bagger"};

struct SBRICK_STRUCT {               // SBrick structure for all data
  boolean UpdateRequest;
  uint8_t Type;
  uint8_t IP;
  String Address;
  String Name;
  uint8_t Mode;
  uint8_t Gradient;
  uint8_t Min;
  uint8_t Max;
  uint8_t Char;
  uint8_t Req[BLE_CHANNEL_MAX];
  uint8_t AxisId[BLE_CHANNEL_MAX];
  uint8_t AxisAxis[BLE_CHANNEL_MAX];
  uint8_t Channel[BLE_CHANNEL_MAX];                //define 4 channels per SBrick
  int Power[BLE_CHANNEL_MAX];
  int PowerLim[BLE_CHANNEL_MAX];
  int PowerTarget[BLE_CHANNEL_MAX];
  bool UpdateReq[BLE_CHANNEL_MAX];
  boolean Sign[BLE_CHANNEL_MAX];
  float Voltage;
  int RSSI;
  uint32_t lastCalcRequest = 0;
  uint32_t offsetCalcRequest = 0;
  uint32_t lastSBrickRequest = 0;
  uint32_t offsetSBrickRequest = 0;
  uint32_t lastVoltageRequest = 0;
  BLEAdvertisedDevice* Device;
  BLERemoteCharacteristic* pCharacteristic_QuickDrive;
  BLERemoteCharacteristic* pCharacteristic_RemoteControl;
  BLEClient* pClient;
};
struct SBRICK_STRUCT SBrick[BLE_MAX]; // define 9 SBricks
String ConnectAddress[BLE_MAX];
int ConnectId[BLE_MAX];
BLEAdvertisedDevice* ConnectDevice[BLE_MAX];
int ConnectCount = 0;

typedef struct
 {
   int x;
   int y;
   bool button;
   unsigned long timestamp;
 }  Sensor;
TaskHandle_t WifiHandle, BleHandle, BleHandle2;
//*****************************************************************************************************************
// ** callback declarations **
void (*AxisCallback)(int id, int axis, int value);
void (*StickCallback)(int id, Sensor value);
void (*ButtonCallback)(int id, bool value);
void (*KeypadCallback)(int id, int value);
