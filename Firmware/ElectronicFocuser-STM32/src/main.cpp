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
//    Serial2.begin(115200);
//    Serial2.begin(57600);
//    Serial2.begin(38400);
    Serial2.begin(115200);

    for(int i = 0; i < 200; i++)
    {
        Serial2.println();
    }
    Serial2.println("Serial2 started.");

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        // Address 0x3D for 128x64
        Serial2.println(F("SSD1306 allocation failed"));
    }

    modbus_f.dataBuffer[REGCOMMANDFROMPI] = 0;
    modbus_f.dataBuffer[REGRESPONSETOPI] = 0;
    modbus_f.dataBuffer[REGSTEPPOSITIONLO] = 0;
    modbus_f.dataBuffer[REGSTEPPOSITIONHI] = 0;
    modbus_f.dataBuffer[REGMAXSTEPSLO] = 0;
    modbus_f.dataBuffer[REGMAXSTEPSHI] = 0;
    modbus_f.dataBuffer[REGMOTORENABLED] = 0;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONLO] = 0;
    modbus_f.dataBuffer[REGTARGETSTEPPOSITIONHI] = 0;
    modbus_f.dataBuffer[REGFOCUSERSPEED] = 0;

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

    GetFlashVar();
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

void GetFlashVar()
{
    focuserFault = EEPROM.read(0);
    Serial2.print("Read focuserFault: ");
    Serial2.println(focuserFault);
}

void SetFlashVar(byte flashValue)
{
    focuserFault = flashValue;
    EEPROM.write(0, focuserFault);
    Serial2.print("Saved focuserFault: ");
    Serial2.println(EEPROM.read(0));
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

unsigned long ReadNumber()
{
    unsigned long charsread = 0;
    unsigned long tempMillis = millis();
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
    Serial2.println("SingleClick() detected.");
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
    Serial2.println("DoubleClick() detected.");
    SignalBeep3();
} // DoubleClick

// this function will be called when the button was pressed multiple times in a short timeframe.
void MultiClick()
{
    int numberOfClicks = button.getNumberClicks();
    Serial2.print("MultiClick(");
    Serial2.print(numberOfClicks);
    Serial2.println(") detected.");
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
            Serial2.println("Focuser to 0.");
            break;

        case 4: // go to middle
            if(focuserFault)
            {
                return;
            }
            SignalBeeps(4, 1500);
            fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
            encoder->setPosition(fstepper.maxSteps / 2 / fstepper.stepRate);
            Serial2.println("Focuser to middle");
            break;

        case 5: // go to max
            if(focuserFault)
            {
                return;
            }
            SignalBeeps(5, 1500);
            fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
            encoder->setPosition(fstepper.maxSteps / fstepper.stepRate + 1);
            Serial2.println("Focuser to max.");
            break;

        case 6: // sync to zero
            if(focuserFault)
            {
                return;
            }
            SyncToZero();
            break;

        case 7: // soft reset
            GetFlashVar();
            notInit = true;
            FocuserInit();
            break;

        case 8: // flash reset
            SetFlashVar(0);
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
                DisplayMessageBacklash();
            }
            break;
    }
} // MultiClick

// this function will be called when the button was held down for 1 second or more.
void PressStart()
{
    // button.tick()
    longPress = true;
    longClickCycles = 0;
    Serial2.println("PressStart()");
    PressStartTime = millis() - 1000; // as set in setPressTicks()
    cycleTime = PressStartTime;
} // PressStart()

// this function will be called when the button was released after a long hold.
void PressStop()
{
    longPress = false;
    Serial2.print("PressStop(");
    Serial2.print(millis() - PressStartTime);
    Serial2.println(") detected.");
    delay(300);
    Signal2Beep2();
} // PressStop()
/////////////////////////////////////////////////////////////////////////////////////////////////////

void FocuserInit()
{
    Serial2.println("");
    Serial2.print("MaxStepsAbsolute: ");
    Serial2.println(fstepper.maxStepsAbsolute);
    Serial2.println("");
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
#ifdef LIMITSWITCHNORMALLYCLOSED
    if(!digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#else // Switch normally open
    if(digitalRead(LIMITSWITCHPIN)) // LIMITSWITCHPIN has reversed logic, switch drives pin to ground.
#endif
        // Stage 1. - Finding zero position
    { // limit switch inactive
        Serial2.println("INIT: 1. Focuser at unknown position...");
        fstepper.maxSteps = fstepper.maxStepsAbsolute + fstepper.maxStepsAbsolute * SAFETYMAXLIMDECREASEPERCENT / 100 / 2;
        fstepper.stepPosition = fstepper.maxSteps;
        fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
        MotorInit();
        fstepper.stepTarget = 0;
        fstepper.PulseStepToTarget();
        lostSteps = 0;
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
        SignalBeeps(2, 1500);
        delay(1000);
    }
    else
    {
        Serial2.println("INIT: 1. Focuser at zero position...");
        fstepper.stepPosition = 0;
        Serial2.print("MaxSteps: ");
        Serial2.print(fstepper.maxSteps);
        Serial2.print("; StepPosition: ");
        Serial2.print(fstepper.stepPosition);
        Serial2.print("; StepTarget: ");
        Serial2.print(fstepper.stepTarget);
        Serial2.print("; LostSteps: ");
        Serial2.println(lostSteps);
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

    // Stage 2. - Going to max. extend position
    DisplayMessageInitStage2();
    fstepper.maxSteps = fstepper.maxStepsAbsolute;
    Serial2.println("");
    Serial2.println("INIT: 2. Going to max. position");
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
        SetFlashVar(1);
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
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps);
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
        SetFlashVar(1);
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
    Serial2.println("");
    Serial2.println("INIT: 3. Approaching zero position");
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed);
    //  fstepper.stepTarget = 20000;
    fstepper.stepTarget = fstepper.maxStepsAbsolute / 8;
    fstepper.PulseStepToTarget();
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps);
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
    Serial2.println("");
    Serial2.println("INIT: 4. Seeking zero position at low speed");
    fstepper.SetFocuserSpeed(fstepper.optimalSpeed - 1);
    fstepper.stepPosition = fstepper.maxStepsAbsolute /
                            4; // raise position to engage the limit switch when going to zero position
    fstepper.stepTarget = 0;
    fstepper.PulseStepToTarget();
    fstepper.maxSteps -= (lostSteps - fstepper.maxStepsAbsolute / 8);
    Serial2.print("MaxSteps: ");
    Serial2.print(fstepper.maxSteps);
    Serial2.print("; StepPosition: ");
    Serial2.print(fstepper.stepPosition);
    Serial2.print("; StepTarget: ");
    Serial2.print(fstepper.stepTarget);
    Serial2.print("; LostSteps: ");
    Serial2.println(lostSteps - (long)fstepper.maxStepsAbsolute / 8);
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
        Serial2.println("Focuser to middle");
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
        Serial2.println("");
        Serial2.println("Zero sync: Going to zero position");
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
        Serial2.println("Zero sync: Focuser at zero position...");
        fstepper.stepPosition = 0;
        Signal2Beep2();
    }
    //  fstepper.stepTarget = initialPosition;
    //  fstepper.PulseStepToTarget();
    DisplayRefresh();
}

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

void DisplayMessageBacklash()
{
    DisplayMessageRegulusFocuser();
    display.setTextSize(1);
    display.println("");
    display.print("Backlash: ");
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
    if( !modbus_f.dataBuffer[REGCOMMANDFROMPI] || focuserFault )
    {
        return;
    }
#ifdef DEBUG
    Serial2.print("Remote Command: ");
    Serial2.print( modbus_f.dataBuffer[REGCOMMANDFROMPI]);
    Serial2.print(" - ");
#endif
    unsigned long relativeSteps;
    switch(modbus_f.dataBuffer[REGCOMMANDFROMPI])
    {
        case 1:
#ifdef DEBUG
            Serial2.println("Speed 1");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 2:
#ifdef DEBUG
            Serial2.println("Speed 2");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;

        case 3:
#ifdef DEBUG
            Serial2.println("Speed 3");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 4:
#ifdef DEBUG
            Serial2.println("Speed 4");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 5:
#ifdef DEBUG
            Serial2.println("Speed 5");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 6:
#ifdef DEBUG
            Serial2.println("Speed 6");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 7:
#ifdef DEBUG
            Serial2.println("Speed 7");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 8:
#ifdef DEBUG
            Serial2.println("Speed 8");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 9:
#ifdef DEBUG
            Serial2.println("Speed 9");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 10:
#ifdef DEBUG
            Serial2.println("Speed 10");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 11:
#ifdef DEBUG
            Serial2.println("Speed 11");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 12:
#ifdef DEBUG
            Serial2.println("Speed 12");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 13:
#ifdef DEBUG
            Serial2.println("Speed 13");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 14:
#ifdef DEBUG
            Serial2.println("Speed 14");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case 15:
#ifdef DEBUG
            Serial2.println("Speed 15");
#endif // DEBUG
            modbus_f.dataBuffer[REGFOCUSERSPEED] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
            fstepper.speedIndex = modbus_f.dataBuffer[REGFOCUSERSPEED];
            fstepper.SetFocuserSpeed(fstepper.speedIndex);
            encoder->setPosition(fstepper.stepPosition / fstepper.stepRate); // recompute encoder position
            DisplayRefresh();
            break;
        case CMDENABLEMOTOR:
#ifdef DEBUG
            Serial2.println("Enable motor");
#endif // DEBUG
            fstepper.EnableMotor(true);
            DisplayRefresh();
            break;
        case CMDDISABLEMOTOR:
#ifdef DEBUG
            Serial2.println("Disable motor");
#endif // DEBUG
            fstepper.EnableMotor(false);
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
            GetFlashVar();
            notInit = true;
            FocuserInit();
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
            DisplayRefresh();
            break;

        case TICKPLUS:
#ifdef DEBUG
            Serial2.println("Tick +");
#endif // DEBUG
            encoder->setPosition(encoder->getPosition() + 1);
            break;
        case TICKMINUS:
#ifdef DEBUG
            Serial2.println("Tick -");
#endif // DEBUG
            encoder->setPosition(encoder->getPosition() - 1);
            break;

        case CMDEXTEND:
#ifdef DEBUG
            Serial2.println("Direction: extend");
#endif // DEBUG
            fstepper.SetMotorDirection(1);
            break;

        case CMDRETRACT:
#ifdef DEBUG
            Serial2.println("Direction: retract");
#endif // DEBUG
            fstepper.SetMotorDirection(-1);
            break;

        case CMDREMOTEENABLE:
#ifdef DEBUG
            Serial2.println("Remote Control enabled");
#endif // DEBUG
            remoteControlEnabled = true;
            encoder->setPosition(fstepper.stepTarget / fstepper.stepRate);
            break;

        case CMDREMOTEDISABLE:
#ifdef DEBUG
            Serial2.println("Remote Control disabled");
#endif // DEBUG
            remoteControlEnabled = false;
            break;

        default:
            Serial2.print("Unknown remote command to execute: ");
            Serial2.println(modbus_f.dataBuffer[REGCOMMANDFROMPI]);
            break;
    }

    modbus_f.dataBuffer[REGRESPONSETOPI] = modbus_f.dataBuffer[REGCOMMANDFROMPI];
    modbus_f.dataBuffer[REGCOMMANDFROMPI] = 0;
} // CommandProcessor()


void loop()
{
    if(notInit && !focuserFault)
    {
        FocuserInit();
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
            Serial2.print("LongPressCycles: ");
            Serial2.println(longClickCycles);
            cycleTime = millis();
            SignalBeep2();
        }
    } // if(remoteControlEnabled)
} // loop