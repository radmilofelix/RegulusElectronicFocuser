//#include <Arduino.h>
#include "ModbusSlave.h"
#include <Adafruit_SSD1306.h>
#include <RotaryEncoder.h>
#include "OneButton.h"
#include "FocuserStepper.h"
#include "FocuserDefinitions.h"

//#define DEBUG

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
void BacklashInterrupt();
void MotorInit();
void FocuserInit(int initMode);
void SyncToZero();
uint32_t Get32FromFlash();
void Set32ToFlash();
void GetFlashFault();
void SetFlashFault(byte faultValue);
void GetFlashPosition();
void SetFlashPosition(uint32_t positionValue);
void GetFlashBacklash();
void SetFlashBacklash(uint32_t backlashValue);
void FindZero();
void WaitForBacklashSwitchClick();
void BacklashTravel(int travelspeed, uint32_t travelTarget);
void BacklashDetect(int travelspeed, uint32_t travelTarget);


#ifdef DEBUG
void DisplayMenu();
void DisplayFocuserData();
unsigned uint32_t ReadNumber();
void DisplayValues();
void KeyboardOperationSelect();
#endif


///////////////////////////////////////////////////////// Messages & Beeps /////////////////////////////////////////////////////
void Signal2Beep1();
void Signal2Beep2();
void SignalBeep1();
void SignalBeep2();
void SignalBeep3();
void SignalBeeps(int nbeeps, int beepFreq);

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
void DisplayMessageInitErrorSlipping();
void DisplayMessageZeroSync();
void DisplayFocuserData();
void DisplayMessageBacklash(uint32_t backlash);
void DisplayMessageBacklashAid1();
void DisplayMessageBacklashAid2();
void DisplayMessageBacklashWaitForInput();
void DisplayMessageBacklashReversing();
void DisplayMessageBacklashDetectionFinished();
void DisplayMessageBacklashDetected(uint32_t value);
void DisplayMessageBacklashMeasurements(uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// Encoder callbacks /////////////////////////////////////////////////////////////
void CheckTicks();
void SingleClick();
void DoubleClick();
void MultiClick();
void PressStart();
void PressStop();
static int pos = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ModbusPoll();
void CommandProcessor();
void BacklashAid();

bool notInit = true;
bool interruptTriggered = false;
//bool remoteControlEnabled = false;
bool remoteControlEnabled = true;
bool backlashInterruptEnable=false;
bool longPress=false;
byte focuserFault = 0;
uint32_t flashPosition;
uint32_t backlash = 0;
uint32_t PressStartTime, cycleTime;
int longClickCycles=0;
int longClickCycleTime=100; // ms
uint32_t lostSteps; // step difference when focuser hits the limit switch at 0 position
int working=0;
unsigned int previousPress = 0;
unsigned int previousBouncePress = 0;
int contactDebounce = 400;
int variableTextSize = 2;
int slipError;
//bool SingleClickToBacklash = false;

RotaryEncoder *encoder = nullptr;
OneButton button(ENCODER_SW, true);
ModbusSlave modbus_f(1,Serial1,PA7);
FocuserStepper fstepper;
