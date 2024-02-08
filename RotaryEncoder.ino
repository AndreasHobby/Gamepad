//*****************************************************************************************************************
// ** header incldes and installation hints **
//*****************************************************************************************************************
#include "AiEsp32RotaryEncoder.h"
//*****************************************************************************************************************
// ** Rotary Encoder configuration **
#define ROTARY_ENCODER_STEPS          4
#define ROTARY_ENCODER_POLLINGTIME    100
#define ROTARY_ENCODER_MAX            1
#define ROTARY_ENCODER_ACCEL          192  //128 //64
const int RotaryPinA[2]   = {12,  4};
const int RotaryPinB[2]   = {13, 15};
const int RotaryPinSw[2]  = {14,  3};
//*****************************************************************************************************************
// ** Rotary Encoder storage data **
AiEsp32RotaryEncoder rotaryEncoder[ROTARY_ENCODER_MAX];
uint32_t RotaryEncoderLastChange = 0;
int RotaryMode[ROTARY_ENCODER_MAX];
bool SuppressUpdate = true;
//*****************************************************************************************************************
//*****************************************************************************************************************
// 
void RotaryReset(int id) {
  RotarySet(id, 0);
}
void RotarySet(int id, int value) {
  if (id >= ROTARY_ENCODER_MAX) return;
  SuppressUpdate = false;
  rotaryEncoder[id].setBoundaries(-POWER_MAX, POWER_MAX, false);
  rotaryEncoder[id].setEncoderValue(value);
  SuppressUpdate = false;
}
void RotaryTest(int id, int value) {
  if (id >= ROTARY_ENCODER_MAX) return;
  rotaryEncoder[id].setBoundaries(-POWER_MAX, POWER_MAX, false);
  rotaryEncoder[id].setEncoderValue(value);
}
//*****************************************************************************************************************
// 
void RotaryInit() {
  for (int i=0; i < ROTARY_ENCODER_MAX; i++){
    rotaryEncoder[i] = AiEsp32RotaryEncoder(RotaryPinA[i], RotaryPinB[i], -1, -1, ROTARY_ENCODER_STEPS);
    pinMode(RotaryPinA[i], INPUT_PULLUP);
    pinMode(RotaryPinB[i], INPUT_PULLUP);
    pinMode(RotaryPinSw[i], INPUT_PULLUP);
    rotaryEncoder[i].begin();
    rotaryEncoder[i].setBoundaries(-POWER_MAX, POWER_MAX, false);
    rotaryEncoder[i].setAcceleration(ROTARY_ENCODER_ACCEL);
    RotaryMode[i] = 0;
  }
    rotaryEncoder[0].setup(
          [] { rotaryEncoder[0].readEncoder_ISR(); },
          [] { ; });
  if (ROTARY_ENCODER_MAX > 1)
    rotaryEncoder[1].setup(
          [] { rotaryEncoder[1].readEncoder_ISR(); },
          [] { ; });
}
//*****************************************************************************************************************
// 
void RotaryCyclicCall() {
  static int ValueOld[ROTARY_ENCODER_MAX];
  static bool RotaryButtonOld[ROTARY_ENCODER_MAX];
  static int RotaryModeOld[ROTARY_ENCODER_MAX];
  static int DiffF[2];
  if (millis() > RotaryEncoderLastChange){
    RotaryEncoderLastChange = millis() + ROTARY_ENCODER_POLLINGTIME;
    for (int i=0; i < ROTARY_ENCODER_MAX; i++){
      bool RotaryButton = !digitalRead(RotaryPinSw[i]);                    //read digital button
      int Value = rotaryEncoder[i].readEncoder();
      int Diff = rotaryEncoder[i].encoderChanged();
      DiffF[i] = DiffF[i] + 0.4 * (Diff - DiffF[i]); //0.6
      if (ControlMainMenu == MAIN_RUN) {
        if (Value < 0) RotaryMode[i] = -1;
        if ((Value == 0) and (Diff == 0) and (DiffF[i] == 0)) RotaryMode[i] = 0;
        if (Value > 0) RotaryMode[i] = 1;
      }
      if (RotaryModeOld[i] != RotaryMode[i]) {
        switch (RotaryMode[i]) {
          case -1:  rotaryEncoder[0].setBoundaries(-POWER_MAX, 0, false);         break;
          case  0:  rotaryEncoder[0].setBoundaries(-POWER_MAX, POWER_MAX, false); break;
          case  1:  rotaryEncoder[0].setBoundaries(0, POWER_MAX, false);          break;
        }
      }
      Sensor a;
      a.y = Value;
      a.x = 0;
      a.button = RotaryButton;
      if ((ValueOld[i] != Value) or (RotaryButtonOld[i] != RotaryButton)) StickCallback(3+i, a);
      if ((ValueOld[i] != Value) and (SuppressUpdate))                    AxisCallback(3+i, 1, Value);
      if (RotaryButtonOld[i] != RotaryButton)                             ButtonCallback(3+i, RotaryButton);
      RotaryButtonOld[i] = RotaryButton;
      ValueOld[i] = Value;
      RotaryModeOld[i] = RotaryMode[i];
      SuppressUpdate = true;
    }
  }
}
