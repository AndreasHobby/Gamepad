//*****************************************************************************************************************
// ** header incldes and installation hints **
#include "Wire.h"
#include "I2CKeyPad.h"
//*****************************************************************************************************************
// ** Keypad configuration **
#define KEYPAD_ADDRESS        0x20
#define KEYPAD_POLLINGTIME    100
//*****************************************************************************************************************
// ** Keypad storage data **
I2CKeyPad keyPad;
uint32_t KeypadLastKeyPressed = 0;
uint8_t idxOld = 16;
//*****************************************************************************************************************
//*****************************************************************************************************************
// 
void KeypadInit() {
//  Wire.begin();
//  Wire.setClock(400000);
  //call it after mpu6050 init !
  if (keyPad.begin(KEYPAD_ADDRESS))  LogAdd("KEYPAD: init done", TFT_GREEN);
}
//*****************************************************************************************************************
// 
void KeypadCyclicCall() {
  if (millis() > KeypadLastKeyPressed){
    KeypadLastKeyPressed = millis() + KEYPAD_POLLINGTIME;
    uint8_t idx = keyPad.getKey();
    //LogAdd("KEYPAD: button pressed: " + String(idx));
    if ((idx < 16) and (idx != idxOld))                KeypadCallback(0, idx);
    idxOld = idx;
  }
}
