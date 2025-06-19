//#include <Arduino.h>
#include "ModbusSlave.h"
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>
#include "OneButton.h"
#include "FocuserStepper.h"
#include "FocuserDefinitions.h"

#define ENCODER_CLK PA6
#define ENCODER_DT PA5
#define ENCODER_SW  PA4
#define BUZZERPIN PA0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//  Use I2C-1
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//  Use I2C-2
TwoWire Wire1(PB11, PB10);// Use STM32 I2C2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);
HardwareSerial Serial2(USART2);   // PA3  (RX)  PA2  (TX)

void setup();
void loop();
void LimitSwitchInterrupt();
void MotorInit();
void FocuserInit();
void SyncToZero();
void GetFlashVar();
void SetFlashVar(byte flashValue);

#ifdef DEBUG
void DisplayMenu();
void DisplayFocuserData();
unsigned long ReadNumber();
void DisplayValues();
void KeyboardOperationSelect();
#endif

void SignalBeeps( int nbeeps, int beepFreq);
void Signal2Beep1();
void Signal2Beep2();
void SignalBeep1();
void SignalBeep2();
void SignalBeep3();

void DisplayRefresh();
void DisplayMessageRegulusFocuser();
void DisplayMessageInitStartMessage();
void DisplayMessageRegulusFocuserInit();
void DisplayMessageInitStage1();
void DisplayMessageInitCanNotReachZero();
void DisplayMessageInitStage2();
void DisplayMessageInitCanNotReachMax();
void DisplayMessageInitStage3();
void DisplayMessageInitEarlyZero();
void DisplayMessageFocuserFault();
void DisplayMessageClearFocuserFault();
void DisplayMessageInitStage4();
void DisplayMessageInitStageFinalSlipError();
void DisplayMessageInitStageFinalBacklash();
void DisplayMessageSlipError();
void DisplayMessageBacklash();
void DisplayMessageInitErrorSlipping();
void DisplayMessageZeroSync();

void DisplayFocuserData();

void ModbusPoll();
void CommandProcessor();

///////////////////////////////////////////////// Encoder callbacks /////////////////////////////////////////////////////////////
void CheckTicks();
void SingleClick();
void DoubleClick();
void MultiClick();
void PressStart();
void PressStop();
static int pos = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool notInit = true;
bool interruptTriggered = false;
bool remoteControlEnabled = true;
byte focuserFault = 0;
unsigned long PressStartTime, cycleTime;
bool longPress=false;
int longClickCycles=0;
int longClickCycleTime=100; // ms
long lostSteps; // step difference when focuser hits the limit switch at 0 position
int working=0;
unsigned int previousPress = 0;
int contactDebounce = 400;
int variableTextSize = 2;
int slipError;

RotaryEncoder *encoder = nullptr;
OneButton button(ENCODER_SW, true);
ModbusSlave modbus_f(1,Serial1,PA7);
FocuserStepper fstepper;
