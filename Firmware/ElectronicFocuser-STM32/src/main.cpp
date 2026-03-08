#include <Arduino.h>
#include "ElectronicFocuser.h"
#include <EEPROM.h>

void setup()
{
    // pin definitions
    pinMode(STDBY, OUTPUT);
    digitalWrite(STDBY, LOW); //device in stand-by
    pinMode(EN, OUTPUT);
    digitalWrite(EN, LOW); // output stage disabled
    pinMode(M1, OUTPUT );
    pinMode(M2, OUTPUT );
    pinMode(M3, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(STEP, OUTPUT);
    pinMode(BUZZERPIN, OUTPUT);
    pinMode(LIMITSWITCHPIN, INPUT_PULLUP); // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
    
    #ifndef DEBUG
    pinMode(TX2PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TX2PIN), BacklashInterrupt, FALLING);
    #endif

    #ifdef LIMITSWITCHNORMALLYCLOSED
    attachInterrupt(digitalPinToInterrupt(LIMITSWITCHPIN), LimitSwitchInterrupt, RISING);
    #else
    attachInterrupt(digitalPinToInterrupt(LIMITSWITCHPIN), LimitSwitchInterrupt, FALLING);
    #endif

    digitalWrite(BUZZERPIN, 0);
    delay (100);
//    modbus_f.begin(115200);
//    modbus_f.begin(57600);
//    modbus_f.begin(38400);
    modbus_f.begin(19200);

    #ifdef DEBUG
//    Serial2.begin(115200);
//    Serial2.begin(57600);
//    Serial2.begin(38400);
    Serial2.begin(115200);

    for(int i = 0; i < 200; i++)
    {
        Serial2.println();
    }
    Serial2.println("Serial2 started.");
    #endif

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        // Address 0x3D for 128x64
        #ifdef DEBUG
        Serial2.println(F("SSD1306 allocation failed"));
        #endif
    }

    modbus_f.dataBuffer[REGCOMMANDFROMDRIVER] = 0;
    modbus_f.dataBuffer[REGRESPONSETODRIVER] = 0;
    modbus_f.dataBuffer[REGSTEPPOSITIONLO] = 0;
    modbus_f.dataBuffer[REGSTEPPOSITIONHI] = 0;
    modbus_f.dataBuffer[REGMAXSTEPSLO] = 0;
    modbus_f.dataBuffer[REGMAXSTEPSHI] = 0;
    modbus_f.dataBuffer[REGMOTORENABLED] = 0;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONLO] = 0;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONHI] = 0;
    modbus_f.dataBuffer[REGFOCUSERSPEED] = 0;
    modbus_f.dataBuffer[REGBACKLASHLO] = 0;
    modbus_f.dataBuffer[REGBACKLASHHI] = 0;

    encoder = new RotaryEncoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR3);
    attachInterrupt(digitalPinToInterrupt(ENCODER_SW), CheckTicks, CHANGE);

    // link the xxxclick functions to be called on xxxclick event.
    button.attachClick(SingleClick);
    button.attachDoubleClick(DoubleClick);
    button.attachMultiClick(MultiClick);
    button.setPressTicks(1000); // that is the time when LongPressStart is called
    button.attachLongPressStart(PressStart);
    button.attachLongPressStop(PressStop);
    fstepper.SelectMicrostepMode(fstepper.microstepping);
    lostSteps = 0;

// Reset flash for debug purposes
//    SetFlashVar(0);

    GetFlashFault();
    GetFlashPosition();
    GetFlashBacklash();

    if(focuserFault)
    {
        DisplayMessageFocuserFault();
        SignalBeeps(6, 1500);
        delay(1000);
        SignalBeeps(6, 1500);
        delay(1000);
        SignalBeeps(6, 1500);
        delay(1000);
        SignalBeeps(6, 1500);
        delay(1000);
    }
} // Setup()

void GetFlashFault()
{
    focuserFault = EEPROM.read(0);
    #ifdef DEBUG
    Serial2.print("Read focuserFault: ");
    Serial2.println(focuserFault);
    #endif
}

void SetFlashFault(byte faultValue)
{
    focuserFault = faultValue;
    EEPROM.write(0, focuserFault);
    #ifdef DEBUG
    Serial2.print("Saved focuserFault: ");
    Serial2.println(EEPROM.read(0));
    #endif
}

uint32_t Get32FromFlash(int position)
{
    return EEPROM.read(position) * 16777216 + EEPROM.read(position+1) * 65536 + EEPROM.read(position+2) * 256 + EEPROM.read(position+3);
}

void Set32ToFlash(int position, uint32_t value)
{
    byte b0 = (value & 0xFF);  // Lowest byte
    byte b1 = ((value >> 8) & 0xFF);
    byte b2 = ((value >> 16) & 0xFF);
    byte b3 = ((value >> 24) & 0xFF);
    EEPROM.write(position, b3);
    EEPROM.write(position+1, b2);
    EEPROM.write(position+2, b1);
    EEPROM.write(position+3, b0);
}

void GetFlashPosition()
{
    flashPosition = Get32FromFlash(1);
}

void SetFlashPosition(uint32_t positionValue)
{
    Set32ToFlash(1, positionValue);
    GetFlashPosition();
}

void GetFlashBacklash()
{
        backlash = Get32FromFlash(5);
}

void SetFlashBacklash(uint32_t backlashValue)
{
    Set32ToFlash(5, backlashValue);
    GetFlashBacklash();
}


void LimitSwitchInterrupt()
{
    if(fstepper.stepPosition == 0)
    {
        return;
    }
#ifdef LIMITSWITCHNORMALLYCLOSED
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
    { // limit switch active
        if( (millis() - previousPress) > contactDebounce )
        {
            if(notInit) // focuser is not initialised
            {
                lostSteps = fstepper.stepPosition;
                #ifdef DEBUG
                Serial2.print("zerolimitPosition: ");
                Serial2.println(lostSteps);
                #endif
                fstepper.stepPosition = 0;
                encoder->setPosition(fstepper.stepPosition / fstepper.stepRate);
                fstepper.stepTarget = 0;
                previousPress = millis();
                interruptTriggered = true;
                #ifdef DEBUG
                Serial2.println("Limit Switch!!!");
                #endif
            }
            else // focuser is initialised and zero position reached
            {
                fstepper.stepPosition = 0;
                encoder->setPosition(fstepper.stepPosition / fstepper.stepRate);
                fstepper.stepTarget = 0;
                fstepper.EnableMotor(false);
            }
            #ifdef DEBUG
            Serial2.println("Limit Switch Debounced!!!");
            #endif
        }
        else // contact not debounced
        {
            #ifdef DEBUG
            Serial2.println("Limit Switch Not Debounced!!!");
            #endif
        }
    }
    else // contacts vibrate
    {
        #ifdef DEBUG
        Serial2.println("Limit Switch contacts vibrate!!!");
        #endif
    }
}

void BacklashInterrupt()
{
    if ( !backlashInterruptEnable )
    {
        return;
    }
    if( !digitalRead(TX2PIN) ) // TX2PIN has reversed logic, switch drives pin to ground.
    { // backlash switch active
        if( (millis() - previousBouncePress) > contactDebounce )
        {
            if(notInit) // focuser is not initialised
            {
                return;
            }
            fstepper.EnableMotor(false);
            fstepper.stepTarget = fstepper.stepPosition;
            backlashInterruptEnable = false;
        }
    }
}

void WaitForBacklashSwitchClick()
{
    while( digitalRead(TX2PIN) );
    return;
}

#ifdef DEBUG
void DisplayMenu()
{
    // Serial monitor menu
    Serial2.println();
    Serial2.println("Keyboard commands:");
    Serial2.println("=========================================================");
    Serial2.println(F(" m = mode"));
    Serial2.println(F(" r = dir retract"));
    Serial2.println(F(" t = dir extend"));
    Serial2.println(F(" e = driver on "));
    Serial2.println(F(" o = driver in stand by"));
    Serial2.println(F(" s = step"));
    Serial2.println(F(" f = frequency for tone drive"));
    Serial2.println(F(" d = delay (us) for pulse drive"));
    Serial2.println(F(" p = pulse drive"));
    Serial2.println(F(" l = tone drive"));
    Serial2.println(F(" c = enableMotor"));
    Serial2.println(F(" b = go to zero"));
    Serial2.println(F(" j = go to middle"));
    Serial2.println(F(" n = go to max"));
    Serial2.println(F(" x = sync to zero"));
    Serial2.println(F(" k = soft reset"));
    Serial2.println(F(" v = display motion values"));
    Serial2.println(F(" q = display focuser data"));
    Serial2.println(F(" z = display this menu"));
    Serial2.println("=========================================================");
    Serial2.println();
    //
}

void DisplayFocuserData()
{
    Serial2.println();
    Serial2.println();
    Serial2.println("Regulus Focuser Data");
    Serial2.println("=========================================================");
    Serial2.print("Program version: ");
    Serial2.println(PROGRAMVERSION, 1);
    Serial2.print("Focuser type: ");
#ifdef SCT_MCT
    Serial2.println("Crayford focuser for SCT and MCT");
#endif
#ifdef PHOTON_254
    Serial2.println("Crayford focuser - Newtonian TS Photon 254");
#endif
    Serial2.print("Motor resolution: ");
    Serial2.print(MOTORRESOLUTION, 1);
    Serial2.println(" degrees/step");
    Serial2.print("Maximal microstepping: ");
    Serial2.print(MAXMICROSTEPPING);
    Serial2.println(" microsteps per step");
    Serial2.print("Gearbox ratio: ");
    Serial2.print(GEARRATIO);
    if(GEARRATIO == 1)
    {
        Serial2.print(" - direct drive");
    }
    Serial2.println();
    Serial2.println("=========================================================");
    Serial2.println();
    Serial2.println("Type \"z\" for menu");
    Serial2.println();
}

unsigned uint32_t ReadNumber()
{
    unsigned uint32_t charsread = 0;
    unsigned uint32_t tempMillis = millis();
    byte chars = 0;
    do
    {
        if (Serial2.available())
        {
            byte tempChar = Serial2.read();
            chars++;
            if ((tempChar >= 48) && (tempChar <= 57))//is it a number?
            {
                charsread = (charsread * 10) + (tempChar - 48);
            }
            else if ((tempChar == 10) || (tempChar == 13))
            {
                //exit at CR/LF
                break;
            }
        }
    }
    while ((millis() - tempMillis < 2000) && (charsread <= 100000) && (chars < 10));
    return (charsread);
}


void DisplayValues()
{
    Serial2.println();
    Serial2.println("Focuser values");
    Serial2.println("=========================================================");
    if(fstepper.motorEnable)
    {
        Serial2.println("Motor enabled");
    }
    else
    {
        Serial2.println("Motor disabled");
    }
    if(fstepper.motorDirection > 0)
    {
        Serial2.println("Motor direction: extend");
    }
    if(fstepper.motorDirection < 0)
    {
        Serial2.println("Motor direction: retract");
    }
    fstepper.ValidateMicrostepMode();
    Serial2.print(F("Number of steps: "));
    Serial2.println(fstepper.steps, DEC);
    Serial2.print(F("Frequency-for tone drive (Hz): "));
    Serial2.println(fstepper.freq, DEC);
    Serial2.print(F("Pulse delay-for pulse drive (us): "));
    Serial2.println(fstepper.halfCycleDuration, DEC);
    Serial2.println();
    Serial2.print("Max. steps: ");
    Serial2.println(fstepper.maxSteps);
    Serial2.print("Current position: ");
    Serial2.println(fstepper.stepPosition);
    Serial2.println("=========================================================");
    Serial2.println();
    Serial2.println("Type \"z\" - for menu");
    Serial2.println();
}

void KeyboardOperationSelect()
{
    //check serial
    if (Serial2.available())
    {
        byte command = Serial2.read();
        switch (command)
        {
            // enable the device ( out of stand by )
            case 'e':
                Serial2.println(" Motor enabled");
                //          motorEnable=1;
                fstepper.EnableMotor(true);
                fstepper.SelectMicrostepMode(fstepper.microstepping);
                //          digitalWrite(STDBY, HIGH);
                delay(1);
                break;
            // disable the device - stand by
            case 'o':
                Serial2.println(" Motor disabled");
                fstepper.EnableMotor(false);
                //          motorEnable=0;
                //          digitalWrite(STDBY, LOW);
                break;
            case 't':
                Serial2.println(" Motor direction: extend");
                fstepper.SetMotorDirection(1);
                break;
            case 'r':
                Serial2.println(" Motor direction: retract");
                fstepper.SetMotorDirection(-1);
                break;
            case 's': //step number
                fstepper.steps = ReadNumber();
                Serial2.print(F(" Number of steps: "));
                Serial2.println(fstepper.steps, DEC);
                break;
            case 'f': // speed number
                fstepper.freq = ReadNumber();
                Serial2.print(F(" Frequency for tone drive (Hz): "));
                Serial2.println(fstepper.freq, DEC);
                break;
            case 'd': // pulse delay
                fstepper.halfCycleDuration = ReadNumber();
                Serial2.print(F(" Pulse delay for pulse drive (us): "));
                Serial2.println(fstepper.halfCycleDuration, DEC);
                break;
            case 'm': //microstepping selection
                fstepper.microstepping = ReadNumber();
                fstepper.ValidateMicrostepMode();
                fstepper.SelectMicrostepMode(fstepper.microstepping);
                //           Serial2.print(F("Microstep mode: "));
                //           Serial2.println(microstepping, DEC);
                break;
            case 'l': // tone drive
                Serial2.println(F(" Driving motor by tone"));
                fstepper.ToneStep(fstepper.steps, fstepper.motorDirection, fstepper.freq);
                fstepper.stepTarget = fstepper.stepPosition;
                DisplayRefresh();
                break;
            case 'p': // pulse drive
                Signal2Beep1();
                Serial2.println(F(" Driving motor by pulse"));
                fstepper.PulseStep(fstepper.steps, fstepper.motorDirection, fstepper.halfCycleDuration);
                fstepper.stepTarget = fstepper.stepPosition;
                delay(200);
                Signal2Beep1();
                DisplayRefresh();
                break;
            case 'b': // go to zero
                Serial2.println(F(" Going to zero"));
                fstepper.stepTarget = 0;
                fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
                fstepper.PulseStepToTarget();
                break;
            case 'j': // go to middle
                Serial2.println(F(" Going to middle"));
                fstepper.stepTarget = fstepper.maxSteps / 2;
                fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
                fstepper.PulseStepToTarget();
                break;
            case 'n': // go to max
                Serial2.println(F(" Going to max"));
                fstepper.stepTarget = fstepper.maxSteps;
                fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
                fstepper.PulseStepToTarget();
                break;
            case 'x': // sync to zero
                Serial2.println(F(" Sync to zero"));
                SyncToZero();
                break;
            case 'k': // Soft reset
                Serial2.println(F(" Soft reset"));
                focuserFault=1;
                FocuserInit();
                break;
            case 'c':
                //           fstepper.enableMotion=true;
                //           Serial2.println("Motion Enabled");
                break;
            case 'z':
                DisplayMenu();
                break;
            case 'v':
                DisplayValues();
                break;
            case 'q':
                DisplayFocuserData();
                break;
        }
    }
}
#endif

void MotorInit()
{
    fstepper.EnableMotor(true);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////   ENCODER CALLBACKS ///////////////////////////////////////////
void CheckTicks()
{
    button.tick(); // just call tick() to check the state.
}

// this function will be called when the button was pressed 1 time only.
void SingleClick()
{
    if(focuserFault)
    {
        return;
    }
    if(fstepper.speedIndex > 1)
    {
        fstepper.speedIndex--;
    }
    fstepper.SetFocuserSpeed(fstepper.speedIndex);
    encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
    DisplayRefresh();
    #ifdef DEBUG
    Serial2.println("SingleClick() detected.");
    #endif
    SignalBeep1();
} // SingleClick

// this function will be called when the button was pressed 2 times in a short timeframe.
void DoubleClick()
{
    if(focuserFault)
    {
        return;
    }
    if(fstepper.speedIndex < 15)
    {
        fstepper.speedIndex++;
    }
    fstepper.SetFocuserSpeed(fstepper.speedIndex);
    encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
    DisplayRefresh();
    #ifdef DEBUG
    Serial2.println("DoubleClick() detected.");
    #endif
    SignalBeep3();
} // DoubleClick

// this function will be called when the button was pressed multiple times in a short timeframe.
void MultiClick()
{
    int numberOfClicks = button.getNumberClicks();
    #ifdef DEBUG
    Serial2.print("MultiClick(");
    Serial2.print(numberOfClicks);
    Serial2.println(") detected.");
    #endif
    switch(numberOfClicks)
    {
        case 3: // go to zero
            if(focuserFault)
            {
                return;
            }
            SignalBeeps(3, 1500);
            fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
            encoder->setPosition(0);
            #ifdef DEBUG
            Serial2.println("Focuser to 0.");
            #endif
            break;

            case 4: // go to middle
            if(focuserFault)
            {
                return;
            }
            SignalBeeps(4, 1500);
            fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
            encoder->setPosition(fstepper.maxSteps / 2 / fstepper.stepRate);
            #ifdef DEBUG
            Serial2.println("Focuser to middle");
            #endif
            SetFlashPosition(fstepper.maxSteps / 2);
            break;

        case 5: // go to max
            if(focuserFault)
            {
                return;
            }
            SignalBeeps(5, 1500);
            fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
            encoder->setPosition(fstepper.maxSteps / fstepper.stepRate + 1);
            #ifdef DEBUG
            Serial2.println("Focuser to max.");
            #endif
            break;

        case 6: // sync to zero
            if(focuserFault)
            {
                return;
            }
            SyncToZero();
            Signal2Beep2();
            break;

        case 7: // soft reset
            GetFlashFault();
            notInit = true;
            FocuserInit(0);
            break;

        case 8: // flash reset
            SetFlashFault(0);
            notInit = false;
            DisplayMessageClearFocuserFault();
            SignalBeeps(3, 1500);
            delay(1000);
            SignalBeeps(3, 1500);
            delay(1000);
            SignalBeeps(3, 1500);
            delay(1000);
            break;

        case 9: // display slip error or backlash
            if( !fstepper.gearedType )
            {
                DisplayMessageSlipError();
            }
            else
            {
                DisplayMessageBacklash(backlash);
            }
            DisplayMessageBacklash(backlash);
            Signal2Beep2();
            break;

        case 10: // Backlash aid
            BacklashAid();
            break;            

        case 11: // Full init
            FocuserInit(0);
            break;            
    }
} // MultiClick

void FindZero(int travelSpeed)
{
    #ifdef LIMITSWITCHNORMALLYCLOSED
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
    #else // Switch normally open
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
    #endif
    { // limit switch inactive    
        fstepper.SetFocuserSpeed(travelSpeed);
        fstepper.stepPosition = fstepper.maxSteps;
        fstepper.stepTarget = 0;
        DisplayRefresh();
        MotorInit();
        fstepper.PulseStepToTarget();
        DisplayRefresh();
    }
}

void BacklashTravel(int travelSpeed, uint32_t travelTarget)
{
    fstepper.SetFocuserSpeed(travelSpeed);
    fstepper.stepTarget = travelTarget;
    DisplayRefresh();
    MotorInit();
    fstepper.PulseStepToTarget();
    DisplayRefresh();
}

void BacklashDetect(int travelSpeed, uint32_t travelTarget)
{
    backlashInterruptEnable = true;
    BacklashTravel(travelSpeed, travelTarget);
}

void BacklashAid()
{
    remoteControlEnabled = false;
    int travelSpeed = fstepper.optimalSpeed+2;
    int slowSpeed = fstepper.optimalSpeed - 3;
    SignalBeeps(10, 1500);
    DisplayMessageBacklashAid1();
    WaitForBacklashSwitchClick();
    delay(1000);
    SignalBeeps(1, 2000);
    DisplayMessageBacklashAid2();
    WaitForBacklashSwitchClick();
    uint32_t meanBacklash = 0;
    uint32_t backlashArray[4];
    bool initBacklash = false;
    for(int i=0; i<4; i++)
    {
        uint32_t instantBaclash1=0;
        uint32_t instantBaclash2=0;
        if( initBacklash )
        {
            DisplayMessageBacklashReversing();
            delay(2000);
        }
        else
        {
            initBacklash = true;
        }
        FindZero(travelSpeed);
        BacklashTravel(travelSpeed, fstepper.maxSteps/2);
        DisplayMessageBacklashWaitForInput();
        SignalBeeps(3, 2000);
        WaitForBacklashSwitchClick();
        delay(1000);
        SignalBeeps(1, 2000);
        BacklashDetect(slowSpeed, fstepper.maxSteps/4);
        instantBaclash1 = fstepper.maxSteps/2 - fstepper.stepPosition;
        meanBacklash = instantBaclash1;
        DisplayMessageBacklashReversing();
        delay(2000);
        BacklashTravel(travelSpeed, fstepper.maxSteps*3/4);
        BacklashTravel(travelSpeed, fstepper.maxSteps/2);
        DisplayMessageBacklashWaitForInput();
        SignalBeeps(3, 2000);
        WaitForBacklashSwitchClick();
        delay(1000);
        SignalBeeps(1, 2000);
        BacklashDetect(slowSpeed, fstepper.maxSteps*3/4);
        instantBaclash2 = fstepper.stepPosition - fstepper.maxSteps/2;
        meanBacklash = instantBaclash2;
        meanBacklash = (instantBaclash1 + instantBaclash2) / 2;
        backlashArray[i] = meanBacklash;
    }
    delay(1000);
    SignalBeeps(3, 1500);
    DisplayMessageBacklashDetectionFinished();
    WaitForBacklashSwitchClick();
    delay(1000);
    DisplayMessageBacklashMeasurements( backlashArray[0], backlashArray[1], backlashArray[2], backlashArray[3] );
    WaitForBacklashSwitchClick();
    delay(1000);
    backlash = ( backlashArray[0] + backlashArray[1] + backlashArray[2] + backlashArray[3]) / 4;
    backlash += backlash * 5 /100;
    SetFlashBacklash( backlash);
    DisplayMessageBacklashDetected(backlash);
    WaitForBacklashSwitchClick();
    BacklashTravel(travelSpeed, fstepper.maxSteps/2);
    encoder->setPosition(fstepper.maxSteps/2 / fstepper.stepRate + 1);
    remoteControlEnabled = true;
    Signal2Beep2();
}



// this function will be called when the button was held down for 1 second or more.
void PressStart()
{
    // button.tick()
    longPress = true;
    longClickCycles = 0;
    #ifdef DEBUG
    Serial2.println("PressStart()");
    #endif
    PressStartTime = millis() - 1000; // as set in setPressTicks()
    cycleTime = PressStartTime;
} // PressStart()

// this function will be called when the button was released after a long hold.
void PressStop()
{
    longPress = false;
    #ifdef DEBUG
    Serial2.print("PressStop(");
    Serial2.print(millis() - PressStartTime);
    Serial2.println(") detected.");
    #endif
    delay(300);
    Signal2Beep2();
} // PressStop()
/////////////////////////////////////////////////////////////////////////////////////////////////////

void FocuserInit(int initMode)
{
    #ifdef DEBUG
    Serial2.println("");
    Serial2.print("MaxStepsAbsolute: ");
    Serial2.println(fstepper.maxStepsAbsolute);
    Serial2.println("");
    #endif
    if(fstepper.maxStepsAbsolute < 1000000)
    {
        variableTextSize = 2;
    }
    else
    {
        variableTextSize = 1;
    }
    delay(1000);
    DisplayMessageInitStartMessage();
    delay(2000);
    SignalBeeps(7, 2500);
    delay(500);

    DisplayMessageInitStage1();
    lostSteps = 0;
    fstepper.SetMotorDirection(-1); // retract

    if(initMode == -1)
    {
        notInit = false;
        fstepper.SetFocuserSpeed(11);
        fstepper.stepPosition = flashPosition;
        fstepper.stepTarget = flashPosition;
        encoder->setPosition(flashPosition / fstepper.stepRate + 1);
        DisplayRefresh();
        return;
    }    

#ifdef LIMITSWITCHNORMALLYCLOSED
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
        // Stage 1. - Finding zero position
    { // limit switch inactive
        #ifdef DEBUG
        Serial2.println("INIT: 1. Focuser at unknown position...");
        #endif
        fstepper.maxSteps = fstepper.maxStepsAbsolute + fstepper.maxStepsAbsolute * SAFETYMAXLIMDECREASEPERCENT / 100 / 2;
        fstepper.stepPosition = fstepper.maxSteps;
        fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
        MotorInit();
        fstepper.stepTarget = 0;
        fstepper.PulseStepToTarget();
        lostSteps = 0;
        #ifdef DEBUG
        Serial2.println("");
        Serial2.println("INIT: 1. Seeking zero position");
        Serial2.print("MaxSteps: ");
        Serial2.print(fstepper.maxSteps);
        Serial2.print("; StepPosition: ");
        Serial2.print(fstepper.stepPosition);
        Serial2.print("; StepTarget: ");
        Serial2.print(fstepper.stepTarget);
        Serial2.print("; LostSteps: ");
        Serial2.println(lostSteps);
        #endif
        SignalBeeps(2, 1500);
        delay(1000);
    }
    else
    {
        fstepper.stepPosition = 0;
        #ifdef DEBUG
        Serial2.println("INIT: 1. Focuser at zero position...");
        Serial2.print("MaxSteps: ");
        Serial2.print(fstepper.maxSteps);
        Serial2.print("; StepPosition: ");
        Serial2.print(fstepper.stepPosition);
        Serial2.print("; StepTarget: ");
        Serial2.print(fstepper.stepTarget);
        Serial2.print("; LostSteps: ");
        Serial2.println(lostSteps);
        #endif
    }

#ifdef LIMITSWITCHNORMALLYCLOSED
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
    { // limit switch inactive
        DisplayMessageInitCanNotReachZero();
        remoteControlEnabled = false;
        notInit = false;
        focuserFault = 1;
        return;
    }

    // Skipping the rest of the init stages for mode = 1
    if(initMode == 1)
    {
        fstepper.SetFocuserSpeed(11);
        fstepper.stepTarget = flashPosition + backlash * 2;
        MotorInit();
        fstepper.PulseStepToTarget();
        fstepper.stepTarget = flashPosition;
        fstepper.PulseStepToTarget();
        encoder->setPosition(flashPosition / fstepper.stepRate + 1);
        DisplayRefresh();
        notInit = false;
        return;
    }

    // Stage 2. - Going to max. extend position
    DisplayMessageInitStage2();
    fstepper.maxSteps = fstepper.maxStepsAbsolute;
    #ifdef DEBUG
    Serial2.println("");
    Serial2.println("INIT: 2. Going to max. position");
    #endif
    MotorInit();
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed);

 // go to maxSteps/10 and check if limit switch is released
    fstepper.stepTarget = fstepper.maxSteps / 10;
    fstepper.PulseStepToTarget();
#ifdef LIMITSWITCHNORMALLYCLOSED
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
    { // limit switch active
        DisplayMessageInitCanNotReachMax();
        remoteControlEnabled = false;
        notInit = false;
        focuserFault = 1;
        SetFlashFault(1);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        return;
    }

    fstepper.stepTarget = fstepper.maxSteps;
    fstepper.PulseStepToTarget();
    #ifdef DEBUG
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps);
    #endif
    SignalBeeps(2, 1500);
    delay(1000);

#ifdef LIMITSWITCHNORMALLYCLOSED
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
    { // limit switch active
        DisplayMessageInitCanNotReachMax();
        remoteControlEnabled = false;
        notInit = false;
        focuserFault = 1;
        SetFlashFault(1);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        SignalBeeps(5, 1500);
        delay(1000);
        return;
    }

    // Stage 3. - Approaching the zero position
    DisplayMessageInitStage3();
    #ifdef DEBUG
    Serial2.println("");
    Serial2.println("INIT: 3. Approaching zero position");
    #endif
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
    //  fstepper.stepTarget = 20000;
    fstepper.stepTarget = fstepper.maxStepsAbsolute / 8;
    fstepper.PulseStepToTarget();
    #ifdef DEBUG
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps);
    #endif
    SignalBeeps(2, 1500);

#ifdef LIMITSWITCHNORMALLYCLOSED
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
    { // limit switch active
        DisplayMessageInitEarlyZero();
        remoteControlEnabled = false;
        notInit = false;
        focuserFault = 1;
        return;
    }

    // Stage 4. - Seek zero position at low speed, correct max. steps
    DisplayMessageInitStage4();
    #ifdef DEBUG
    Serial2.println("");
    Serial2.println("INIT: 4. Seeking zero position at low speed");
    #endif
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed - 1);
    fstepper.stepPosition = fstepper.maxStepsAbsolute /
                            4; // raise position to engage the limit switch when going to zero position
    fstepper.stepTarget = 0;
    fstepper.PulseStepToTarget();
    fstepper.maxSteps -= (lostSteps - fstepper.maxStepsAbsolute / 8);
    #ifdef DEBUG
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps - (long)fstepper.maxStepsAbsolute / 8);
    #endif
    SignalBeeps(2, 1500);
    delay(1000);

    MotorInit();
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed - 1);
    slipError = abs(((double)fstepper.maxSteps - fstepper.maxStepsAbsolute));
    if( !fstepper.gearedType )
    {
        DisplayMessageInitStageFinalSlipError();
        if( slipError > fstepper.maxStepsAbsolute / 10 ) // admissible error due to slipping: 10%
        {
            notInit = false;
            focuserFault = 1;
            DisplayMessageInitErrorSlipping();
            remoteControlEnabled = false;
            SignalBeeps(5, 1500);
            delay(1000);
            SignalBeeps(5, 1500);
            delay(1000);
            SignalBeeps(5, 1500);
            delay(1000);
            return;
        }
    }
    else // geared type focuser
    {
        slipError /= 2;
        fstepper.backlash = slipError;
        DisplayMessageInitStageFinalBacklash();
        if( slipError > MAXGEAREDBACKLASH )
        {
            notInit = false;
            focuserFault = 1;
            return;
        }
    }
        focuserFault = 0;
        // Go to middle
        fstepper.stepTarget = fstepper.maxSteps / 2;
        fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
        fstepper.PulseStepToTarget();
        encoder->setPosition(fstepper.maxSteps / 2 / fstepper.stepRate);
        Signal2Beep2();
        #ifdef DEBUG
        Serial2.println("Focuser to middle");
        #endif
        DisplayRefresh();
#ifdef DEBUG
        DisplayMenu();
#endif
    notInit = false;
} // FocuserInit() //////////////////////////////////////////////////////////////////////////////////

void SyncToZero()
{
    DisplayMessageZeroSync();
    //  long initialPosition = fstepper.stepPosition;
#ifdef LIMITSWITCHNORMALLYCLOSED
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
        // Finding zero position
    {
        #ifdef DEBUG
        Serial2.println("");
        Serial2.println("Zero sync: Going to zero position");
        #endif
        fstepper.maxSteps = fstepper.maxStepsAbsolute + fstepper.maxStepsAbsolute / 10;
        fstepper.stepPosition = fstepper.maxSteps;
        fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
        fstepper.stepTarget = 0;
        fstepper.PulseStepToTarget();
        lostSteps = 0;
        SignalBeeps(2, 1500);
        delay(1000);
    }
    else
    {
        #ifdef DEBUG
        Serial2.println("Zero sync: Focuser at zero position...");
        #endif
        fstepper.stepPosition = 0;
        Signal2Beep2();
    }
    //  fstepper.stepTarget = initialPosition;
    //  fstepper.PulseStepToTarget();
    DisplayRefresh();
}

void ModbusPoll()
{
    modbus_f.dataBuffer[REGSTEPPOSITIONLO] = fstepper.stepPosition & 65535;
    modbus_f.dataBuffer[REGSTEPPOSITIONHI] = fstepper.stepPosition >> 16;
    modbus_f.dataBuffer[REGMAXSTEPSLO] = fstepper.maxSteps & 65535;
    modbus_f.dataBuffer[REGMAXSTEPSHI] = fstepper.maxSteps >> 16;
    modbus_f.dataBuffer[REGMOTORENABLED] = fstepper.motorEnable;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONLO] = fstepper.stepTarget & 65535;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONHI] = fstepper.stepTarget >> 16;
    modbus_f.dataBuffer[REGFOCUSERSPEED] = fstepper.speedIndex;
    modbus_f.dataBuffer[REGREMOTECONTROL] = remoteControlEnabled;
    modbus_f.dataBuffer[REGFAULT] = focuserFault;
    modbus_f.dataBuffer[REGBACKLASHLO] = backlash & 65535;
    modbus_f.dataBuffer[REGBACKLASHHI] = backlash >> 16;

    if(fstepper.motorDirection > 0)
    {
        modbus_f.dataBuffer[REGDIRECTION] = 1;
    }
    else
    {
        modbus_f.dataBuffer[REGDIRECTION] = 2;
    }
    modbus_f.poll(NUMBEROFREGISTERS);
}

void CommandProcessor()
{
    uint32_t newFlashPosition;
    if( !modbus_f.dataBuffer[REGCOMMANDFROMDRIVER] || focuserFault )
    {
        return;
    }
    #ifdef DEBUG
    Serial2.print("Remote Command: ");
    Serial2.print( modbus_f.dataBuffer[REGCOMMANDFROMDRIVER]);
    Serial2.print(" - ");
    #endif
    unsigned long relativeSteps;
    switch(modbus_f.dataBuffer[REGCOMMANDFROMDRIVER])
    {
        case CMDSPEED1:
            #ifdef DEBUG
            Serial2.println("Speed 1");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED2:
            #ifdef DEBUG
            Serial2.println("Speed 2");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            Signal2Beep2();
            break;
            
        case CMDSPEED3:
            #ifdef DEBUG
            Serial2.println("Speed 3");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED4:
            #ifdef DEBUG
            Serial2.println("Speed 4");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED5:
            #ifdef DEBUG
            Serial2.println("Speed 5");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED6:
            #ifdef DEBUG
            Serial2.println("Speed 6");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED7:
            #ifdef DEBUG
            Serial2.println("Speed 7");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED8:
            #ifdef DEBUG
            Serial2.println("Speed 8");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED9:
            #ifdef DEBUG
            Serial2.println("Speed 9");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED10:
            #ifdef DEBUG
            Serial2.println("Speed 10");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED11:
            #ifdef DEBUG
            Serial2.println("Speed 11");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED12:
            #ifdef DEBUG
            Serial2.println("Speed 12");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED13:
            #ifdef DEBUG
            Serial2.println("Speed 13");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED14:
            #ifdef DEBUG
            Serial2.println("Speed 14");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDSPEED15:
            #ifdef DEBUG
            Serial2.println("Speed 15");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDENABLEMOTOR:
            #ifdef DEBUG
            Serial2.println("Enable motor");
            #endif // DEBUG
            fstepper.EnableMotor(true);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDDISABLEMOTOR:
            #ifdef DEBUG
            Serial2.println("Disable motor");
            #endif // DEBUG
            fstepper.EnableMotor(false);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDINIT:
            #ifdef DEBUG
            Serial2.println("Focuser init");
            #endif // DEBUG

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// init fails if called from here. Check the values of the FocuserStepper class members set by the constructor
//  
//     set notInit = true before calling FocuserInit
//
            GetFlashFault();
            notInit = true;
            FocuserInit(1);
            break;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        case CMDSETSPEED:
            #ifdef DEBUG
            Serial2.println("Set speed");
            #endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = 12;
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDGOTOZERO:
            #ifdef DEBUG
            Serial2.println("Goto zero");
            #endif // DEBUG
            fstepper.SetFocuserSpeed(12);
            fstepper.stepPosition = fstepper.maxSteps;
            fstepper.stepTarget = 0;
            fstepper.PulseStepToTarget();
            encoder->setPosition(0);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDGOTOMAX:
            #ifdef DEBUG
            Serial2.println("Goto max position");
            #endif // DEBUG
            fstepper.SetFocuserSpeed(12);
            fstepper.stepPosition = 0;
            fstepper.stepTarget = fstepper.maxSteps;
            fstepper.PulseStepToTarget();
            encoder->setPosition(fstepper.maxSteps / fstepper.stepRate + 1);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDGOTOPOSITION:
            fstepper.stepTarget = modbus_f.dataBuffer[REGREQUESTEDPOSITIONLO] + modbus_f.dataBuffer[REGREQUESTEDPOSITIONHI] * 65536;
            #ifdef DEBUG
            Serial2.print("Goto position: ");
            Serial2.println(fstepper.stepTarget);
            #endif // DEBUG
            remoteControlEnabled = false;
            relativeSteps = abs(fstepper.stepTarget - fstepper.stepPosition);
            fstepper.CorrelateSpeed(relativeSteps);
            fstepper.PulseStepToTarget();
            Signal2Beep2();
            DisplayRefresh();
            break;

        case CMDGOTORELATIVE:
            unsigned long relativeSteps;
            relativeSteps = modbus_f.dataBuffer[REGREQUESTEDPOSITIONLO] + modbus_f.dataBuffer[REGREQUESTEDPOSITIONHI] * 65536;
            #ifdef DEBUG
            Serial2.print("Goto Relative position: ");
            Serial2.println(relativeSteps);
            #endif // DEBUG
            remoteControlEnabled = false;
            fstepper.EnableMotor(true);
            fstepper.CorrelateSpeed(relativeSteps);
            fstepper.RelativePulseStepToTarget(relativeSteps);
            Signal2Beep2();
            DisplayRefresh();
            break;

        case TICKPLUS:
            #ifdef DEBUG
            Serial2.println("Tick +");
            #endif // DEBUG
            encoder->setPosition(encoder->getPosition() + 1);
            Signal2Beep2();
            break;

        case TICKMINUS:
            #ifdef DEBUG
            Serial2.println("Tick -");
            #endif // DEBUG
            encoder->setPosition(encoder->getPosition() - 1);
            Signal2Beep2();
            break;

        case CMDEXTEND:
            #ifdef DEBUG
            Serial2.println("Direction: extend");
            #endif // DEBUG
            fstepper.SetMotorDirection(1);
            Signal2Beep2();
            break;

        case CMDRETRACT:
            #ifdef DEBUG
            Serial2.println("Direction: retract");
            #endif // DEBUG
            fstepper.SetMotorDirection(-1);
            Signal2Beep2();
            break;

        case CMDREMOTEENABLE:
            #ifdef DEBUG
            Serial2.println("Remote Control enabled");
            #endif // DEBUG
            remoteControlEnabled = true;
            encoder->setPosition(fstepper.stepTarget / fstepper.stepRate);
            Signal2Beep2();
            break;

        case CMDREMOTEDISABLE:
            #ifdef DEBUG
            Serial2.println("Remote Control disabled");
            #endif // DEBUG
            remoteControlEnabled = false;
            Signal2Beep2();
            break;

        case CMDSETPOSITION:
            newFlashPosition = modbus_f.dataBuffer[REGFLASHPOSITIONLO] + modbus_f.dataBuffer[REGFLASHPOSITIONHI] * 65536;
            SetFlashPosition(newFlashPosition);
            Signal2Beep2();
        break;

        default:
            #ifdef DEBUG
            Serial2.print("Unknown remote command to execute: ");
            Serial2.println(modbus_f.dataBuffer[REGCOMMANDFROMDRIVER]);
            #endif
            break;
    }

    modbus_f.dataBuffer[REGRESPONSETODRIVER] = modbus_f.dataBuffer[REGCOMMANDFROMDRIVER];
    modbus_f.dataBuffer[REGCOMMANDFROMDRIVER] = 0;
} // CommandProcessor()

void loop()
{
    if(notInit && !focuserFault)
    {
        FocuserInit(1);
    }
    ModbusPoll();
    CommandProcessor();
#ifdef DEBUG
    KeyboardOperationSelect();
#endif
    if(remoteControlEnabled)
    {
        if(fstepper.stepPosition != fstepper.stepTarget)
        {
            bool endBeeps = false;
            if(abs(fstepper.stepPosition - fstepper.stepTarget) > 10000)
            {
                endBeeps = true;
            }
            MotorInit();
            DisplayRefresh();
            if(!focuserFault)
            {
                fstepper.PulseStepToTarget();
            }
            if(endBeeps)
            {
                Signal2Beep2();
            }
            DisplayRefresh();
        }
        if( (fstepper.stepPosition == 0) & interruptTriggered)
        {
            SignalBeeps(2, 2000);
            DisplayRefresh();
            interruptTriggered = false;
        }
        encoder->tick(); // just call tick() to check the state.
        int newPos = encoder->getPosition();
        if (pos != newPos)
        {
            if(newPos >= 0)
            {
                pos = newPos;
            }
            else
            {
                newPos = 0;
                encoder->setPosition(0);
            }
            int maxPos = fstepper.maxSteps / fstepper.stepRate + 1;
            if(newPos <= maxPos)
            {
                pos = newPos;
            }
            else
            {
                encoder->setPosition(maxPos);
            }
            fstepper.stepTarget = pos * fstepper.stepRate;
            if(fstepper.stepTarget > fstepper.maxSteps)
            {
                fstepper.stepTarget = fstepper.maxSteps;
            }
            if(fstepper.stepTarget < 0)
            {
                fstepper.stepTarget = 0;
            }
        } // if  (pos != newPos)

        // keep watching the push button, even when no interrupt happens:
        button.tick();
        if( (millis() - cycleTime > longClickCycleTime) && longPress)
        {
            if(fstepper.speedIndex < 15)
            {
                fstepper.speedIndex++;
            }
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            longClickCycles++;
            #ifdef DEBUG
            Serial2.print("LongPressCycles: ");
            Serial2.println(longClickCycles);
            #endif
            cycleTime = millis();
            SignalBeep2();
        }
    } // if(remoteControlEnabled)
} // loop


/////////////////////////////// Messages & Beeps ///////////////////////////////
void DisplayRefresh()
{
    display.clearDisplay();
    display.setTextSize(variableTextSize);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Pos:");
    display.println(fstepper.stepPosition);
    display.print("Tgt:");
    display.println(fstepper.stepTarget);
    display.print("Lim:");
    display.println(fstepper.maxSteps);
    display.print("Speed: ");
    display.println(fstepper.speedIndex);
    display.display();
}

void DisplayMessageRegulusFocuser()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  Regulus");
    display.println("  Focuser");
}

void DisplayMessageInitStartMessage()
{
    DisplayMessageRegulusFocuser();
    display.println("");
    display.println("INIT...");
    display.display();
}

void DisplayMessageRegulusFocuserInit()
{
    DisplayMessageRegulusFocuser();
    display.println("INIT...");
}

void DisplayMessageInitStage1()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("MaxStepsAbs: ");
    display.println(fstepper.maxStepsAbsolute);
    display.println("Stage 1: Finding zero");
    display.display();
}

void DisplayMessageInitCanNotReachZero()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  ERROR!");
    display.setTextSize(1);
    display.println("Can not reach zero");
    display.println("    position!");
    display.println("");
    display.println("Check 12V supply!");
    display.println("Check mechanics!");
    display.println("");
    display.display();
}

void DisplayMessageInitStage2()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("maxSteps: ");
    display.println(fstepper.maxSteps);
    display.println("Stage 2: GoTo max");
    display.display();
}

void DisplayMessageInitCanNotReachMax()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  ERROR!");
    display.setTextSize(1);
    display.println("Can not reach max");
    display.println("    position!");
    display.println("");
    display.println("Check 12V supply!");
    display.println("Check mechanics!");
    display.println("");
    display.display();
}

void DisplayMessageInitStage3()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("maxSteps: ");
    display.println(fstepper.maxSteps);
    display.println("Stage 3: GoTo zero");
    display.display();
}

void DisplayMessageInitEarlyZero()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  ERROR!");
    display.setTextSize(1);
    display.println("Early reach of zero");
    display.println("    position!");
    display.println("");
    display.println("Check 12V supply!");
    display.println("Check mechanics!");
    display.println("");
    display.display();
}

void DisplayMessageFocuserFault()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  ERROR!");
    display.println(" Focuser");
    display.println("  FAULT!");
    display.setTextSize(1);
    display.println("");
    display.println("Clear flash mem. err.");
    display.display();
}

void DisplayMessageClearFocuserFault()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  CLEARED");
    display.println(" Focuser");
    display.println("  FAULT!");
    display.setTextSize(1);
    display.println("");
    display.println("Reset device.");
    display.display();
}

void DisplayMessageInitStage4()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("maxSteps: ");
    display.println(fstepper.maxSteps);
    display.println("Stage 4: Slow to zero");
    display.display();
}

void DisplayMessageInitStageFinalSlipError()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("Slip Error: ");
    display.println(slipError);
    display.println("End, go to middle");
    display.display();
}

void DisplayMessageInitStageFinalBacklash()
{
    DisplayMessageRegulusFocuserInit();
    display.setTextSize(1);
    display.print("Backlash: ");
    display.println(slipError);
    display.println("End, go to middle");
    display.display();
}

void DisplayMessageSlipError()
{
    DisplayMessageRegulusFocuser();
    display.setTextSize(1);
    display.println("");
    display.print("Slip Error: ");
    display.println(slipError);
    display.display();
}

void DisplayMessageInitErrorSlipping()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  ERROR!");
    display.setTextSize(1);
    display.println("Focuser is slipping!");
    display.println("");
    display.println("Adjust screws!");
    display.println("Check 12V supply!");
    display.println("Check mechanics!");
    display.println("");
    display.display();
}

void DisplayMessageZeroSync()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  Regulus");
    display.println("  Focuser");
    display.println("Zero Sync");
    display.setTextSize(1);
    display.print("MaxStepsAbs: ");
    display.println(fstepper.maxStepsAbsolute);
    display.println("Finding zero...");
    display.display();
}

void DisplayMessageBacklash(uint32_t backlash)
{
    DisplayMessageRegulusFocuser();
//    display.setTextSize(1);
//    display.println("");
//    display.print("Backlash: ");
//    display.println(slipError);
//    display.display();

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.setTextSize(1);
    display.println();
    display.println("Backlash value:");
    display.println(backlash);
    display.println();
    display.println("FlashPos value:");
    display.println(flashPosition);
    display.display();
}

void DisplayMessageBacklashAid1()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.println("    AID");
    display.setTextSize(1);
    display.println("Connect switch to");
    display.println("the Tx pin.");
    display.println("  Press switch");
    display.println("   to continue...");
    display.display();
}

void DisplayMessageBacklashAid2()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.println();
    display.setTextSize(1);
    display.println("8 measurements");
    display.println("will be made.");
    display.println("  Press switch");
    display.println("   to continue...");
    display.display();
}

void DisplayMessageBacklashWaitForInput()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.setTextSize(1);
    display.println("Motor will move slow,");
    display.println("press the button");
    display.println("on the switch when");
    display.println("wheel starts moving.");
    display.println("    Press switch");
    display.println("      to start...");
    display.display();
}

void DisplayMessageBacklashReversing()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
     display.println("");
    display.println("Reversing");
    display.println("direction");
    display.display();
}

void DisplayMessageBacklashDetectionFinished()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.println("Detection");
    display.println("finished!");
    display.setTextSize(1);
    display.println("Press switch...");
    display.display();
}

void DisplayMessageBacklashDetected(uint32_t value)
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.setTextSize(1);
    display.println("Backlash value:");
    display.println(value);
    display.println();
    display.println("Backlash stored");
    display.println("into flash memory.");
    display.println("Press switch...");
    display.display();
}

void DisplayMessageBacklashMeasurements(uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4)
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("  BACKLASH");
    display.setTextSize(1);
    display.println("Detected values:");
    display.println(value1);
    display.println(value2);
    display.println(value3);
    display.println(value4);
    display.println("Press switch...");
    display.display();
}

void SignalBeeps( int nbeeps, int beepFreq)
{
    //  return;
    for(int i = 0; i < nbeeps; i++)
    {
        tone(BUZZERPIN, beepFreq, 150);
        delay(200);
    }
}

void Signal2Beep1()
{
    //  return;
    tone(BUZZERPIN, 1000, 200);
    delay(150);
    tone(BUZZERPIN, 1200, 300);
    delay(150);
}

void Signal2Beep2()
{
    //  return;
    tone(BUZZERPIN, 1200, 200);
    delay(150);
    tone(BUZZERPIN, 1000, 300);
    delay(150);
}

void SignalBeep1()
{
    //  return;
    tone(BUZZERPIN, 2300, 150);
    delay(150);
}

void SignalBeep2()
{
    //  return;
    tone(BUZZERPIN, 1500, 150);
    delay(150);
}

void SignalBeep3()
{
    //  return;
    tone(BUZZERPIN, 1000, 600);
    delay(150);
}


