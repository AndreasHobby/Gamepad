//*****************************************************************************************************************
// ** header incldes and installation hints **
//*****************************************************************************************************************
// ** Analog stick configuration **
#define AnalogResolution    0.125
#define AnalogFilter        0.8
#define AnalogMin           0
#define AnalogMax           511
#define AnalogUpdateJitter  12 //16
#define AnalogDeadzone      16 //24  //16/32
#define AnalogTimeDelay     100
#define AnalogStickMax      2
const int PinA[2]   = {35, 33};
const int PinB[2]   = {34, 32};
const int PinSw[2]  = {25, 26};
//*****************************************************************************************************************
// ** Analog stick storage data **
typedef struct {
   float x;
   float y;
 } XY;
Sensor Analog[2];
Sensor AnalogOld[2];
XY AnalogF[2];
XY AnalogRaw[2];
XY AnalogRawNoJitter[2];
XY AnalogRef[2];
//*****************************************************************************************************************
//*****************************************************************************************************************
// Analog stick init
void AnalogInit() {
  pinMode(PinSw[0], INPUT_PULLUP);
  pinMode(PinA[0], INPUT);
  pinMode(PinB[0], INPUT);
  pinMode(PinSw[1], INPUT_PULLUP);
  pinMode(PinA[1], INPUT);
  pinMode(PinB[1], INPUT);
  int x0=0, y0=0, x1=0, y1=0;
  for (int i=0; i < 16; i++){                                       //read 16 values and find the mean value to define the mid
    x0 = x0 + (analogRead(PinA[0])*AnalogResolution);
    y0 = y0 + analogRead(PinB[0])*AnalogResolution;
    x1 = x1 + (analogRead(PinA[1])*AnalogResolution);
    y1 = y1 + analogRead(PinB[1])*AnalogResolution;
    delay(10);
  }
  AnalogRef[0].x = x0/16;
  AnalogRef[0].y = y0/16;
  AnalogRef[1].x = x1/16;
  AnalogRef[1].y = y1/16;
  LogAdd("STICK: calibration done", TFT_GREEN);
  Sensor a;
  a.x = 0;
  a.y = 0;
  a.button = false;
  for (int j=0; j < AXIS_MAX; j++)     TftPrintAnalogStickInfo(j, a, -1);
}
//*****************************************************************************************************************
// Analog stick polling
void AnalogCyclicCall() {
  if (millis() > Analog[0].timestamp){                              //polling mode, sticks are checked only once per time frame
    for (int i=0; i < AnalogStickMax; i++){
      // READ RAW DATA
      Analog[i].button = !digitalRead(PinSw[i]);                    //read digital button
      AnalogRaw[i].x   = analogRead(PinA[i])*AnalogResolution;      //read analog x-axis
      AnalogRaw[i].y   = analogRead(PinB[i])*AnalogResolution;      //read analog y-axis
      // minimize JITTER
      if ((abs(AnalogRaw[i].x - AnalogRawNoJitter[i].x) > AnalogUpdateJitter) or (abs(AnalogRaw[i].y - AnalogRawNoJitter[i].y) > AnalogUpdateJitter)){
        AnalogRawNoJitter[i].x      = AnalogRaw[i].x;               //take over values only if input is changed greater as AnalogUpdateJitter
        AnalogRawNoJitter[i].y      = AnalogRaw[i].y;               //take over values only if input is changed greater as AnalogUpdateJitter
      }
      // FILTERING (simple PT1 filter)
      AnalogF[i].x = AnalogF[i].x + AnalogFilter * (AnalogRawNoJitter[i].x - AnalogF[i].x);
      AnalogF[i].y = AnalogF[i].y + AnalogFilter * (AnalogRawNoJitter[i].y - AnalogF[i].y);
      // DEAD ZONE COMPENSATION (re-map values to min/mid-x  / mid+x/max)
      if (AnalogF[i].x > AnalogRef[i].x+AnalogDeadzone){
        Analog[i].x = map(AnalogF[i].x, AnalogRef[i].x+AnalogDeadzone, AnalogMax, POWER_MIN, POWER_MAX);
      }else if (AnalogF[i].x < AnalogRef[i].x-AnalogDeadzone){
        Analog[i].x = -map(AnalogF[i].x, AnalogMin, AnalogRef[i].x-AnalogDeadzone, POWER_MAX, POWER_MIN);
      }else{
        Analog[i].x = 0;
      }
      if (AnalogF[i].y > AnalogRef[i].y+AnalogDeadzone){
        Analog[i].y = map(AnalogF[i].y, AnalogRef[i].y+AnalogDeadzone, AnalogMax, POWER_MIN, POWER_MAX);
      }else if (AnalogF[i].y < AnalogRef[i].y-AnalogDeadzone){
        Analog[i].y = -map(AnalogF[i].y, AnalogMin, AnalogRef[i].y-AnalogDeadzone, POWER_MAX, POWER_MIN);
      }else{
        Analog[i].y = 0;
      }
      // INVERT Y axis
      Analog[i].y = -Analog[i].y;
      Analog[i].timestamp = millis() + AnalogTimeDelay;           //set time stamp (only used for sensor 0 to define the polling time)
      // CALL callback functions                                  //only if a value is changed
      if (Analog[i].x != AnalogOld[i].x)                                                                                   AxisCallback(i+1, 0, Analog[i].x);
      if (Analog[i].y != AnalogOld[i].y)                                                                                   AxisCallback(i+1, 1, Analog[i].y);
      if (Analog[i].button != AnalogOld[i].button)                                                                         ButtonCallback(i+1, Analog[i].button);
      if ((Analog[i].x != AnalogOld[i].x) or (Analog[i].y != AnalogOld[i].y) or (Analog[i].button != AnalogOld[i].button)) StickCallback(i+1, Analog[i]);
      memcpy(AnalogOld, Analog, sizeof AnalogOld);               //save old values to make it work
    }
  }
}
