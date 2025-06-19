#include "FocuserStepper.h"


FocuserStepper::FocuserStepper()
{
	motorDirection = -1;
  coilConfiguration = 1; // normal
//  coilConfiguration = -1; // reversed

	steps=0;
	freq=0;
	halfCycleDuration=10;
	microstepping=MAXMICROSTEPPING;
	ms1 = 0;
	ms2 = 0;
	ms3 = 0;
    gearedType = GEAREDTYPE;
    backlash = 0;
    maxStepsAbsolute = FOCUSERTRAVEL * 360 * MAXMICROSTEPPING / MOTORRESOLUTION / FOCUSERTRAVELPER360 * GEARRATIO;
    maxStepsAbsolute -= maxStepsAbsolute*SAFETYMAXLIMDECREASEPERCENT/100; // decrease value
    maxSteps = maxStepsAbsolute;
    stepTarget = 0;
    stepRate = 10000;
    motorEnable = false;
    speedIndex = 14;
    optimalSpeed = 13 - MOTORRESOLUTION / 0.9;
	//motorDriver = DRIVER_A4988;
	//motorDriver = DRIVER_DRV8825;
	motorDriver = DRIVERSTSPIN820;
	//motorDriver = DRIVER_STSPIN220;
 
  if(motorDriver == DRIVERA4988 || motorDriver == DRIVERDRV8825)
  {
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);
  }

#ifdef LIMITSWITCHNORMALLYCLOSED
  if(!digitalRead(LIMITSWITCHPIN))
#else // Switch normally open
  if(digitalRead(LIMITSWITCHPIN))
#endif
  {
    stepPosition=0;
  }
  else
  {
    stepPosition=maxSteps;
    ValidateMicrostepMode();
  }
}

FocuserStepper::~FocuserStepper()
{
	
}

int FocuserStepper::ToneStep (unsigned long numberOfSteps, int motionDirection, unsigned long frequency)
{
  if(!motorEnable)
  {
    Serial2.println("Motion disabled!");
    return 0;
  }
  if(motorDirection != motionDirection)
    SetMotorDirection(motionDirection);
  delay(10);
  long requestedPosition;
  if(motionDirection < 0)
    requestedPosition = stepPosition - numberOfSteps;
  else
    requestedPosition = stepPosition + numberOfSteps;
  if( ( ( requestedPosition < 0) && motionDirection < 0) || (( requestedPosition > maxSteps) && motionDirection > 0) )
  {
    Serial2.println("Tone pulse: position out of range!");
    return 0;
  }
  unsigned long duration=1000*(unsigned long)numberOfSteps/(unsigned long)frequency;
  if (numberOfSteps != 0)
    tone(STEP,(unsigned int)frequency,duration);
  else
    tone(STEP, (unsigned int) frequency ); // turn on, no time limit
  stepPosition += motorDirection*numberOfSteps;
  return 1;
}

int FocuserStepper::PulseStep (unsigned long numberOfSteps, int motionDirection, int duration)
{
  if(!motorEnable)
  {
    Serial2.println("Motion disabled!");
    return 0;
  }
  delay(10);
  for ( int i = 0; i < numberOfSteps; i++)
  {
    if( (stepPosition == 0 && motionDirection < 0) || (stepPosition >= maxSteps && motionDirection > 0) )
      break;
    stepPosition += motorDirection;
    digitalWrite(STEP, HIGH);
    delayMicroseconds(duration);
    digitalWrite(STEP, LOW); 
    delayMicroseconds(duration);
  }
  return 1;
}

void FocuserStepper::RelativePulseStepToTarget(unsigned long relativeSteps)
{
  if(!motorEnable)
  {
    Serial2.println("Motion disabled!");
    return;
  }
  delay(10);

	if(motorDirection > 0)
	{
		if( (stepPosition + relativeSteps) > maxSteps )
			relativeSteps = maxSteps - stepPosition;
		stepPosition += relativeSteps; 
	}
	else
	{
		if( (stepPosition - relativeSteps) < 0 )
			relativeSteps = stepPosition;
		stepPosition -= relativeSteps;
	}
  stepTarget = stepPosition;

  while(relativeSteps)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(halfCycleDuration);
    digitalWrite(STEP, LOW); 
    delayMicroseconds(halfCycleDuration);
    relativeSteps--;
  }
  return;
}

void FocuserStepper::PulseStepToTarget()
{
  if(!motorEnable)
  {
    Serial2.println("Motion disabled!");
    return;
  }
  delay(10);

  while( (stepPosition != stepTarget) && (stepPosition >= 0) && (stepPosition <= maxSteps) )
  {
    if( (stepTarget > stepPosition) && motorDirection < 0)
      SetMotorDirection(1);
    if( (stepTarget < stepPosition) && motorDirection > 0)
      SetMotorDirection(-1);
    stepPosition += motorDirection;
    if(stepPosition < 0)
    {
      stepPosition = 0;
      return;
    }
    digitalWrite(STEP, HIGH);
    delayMicroseconds(halfCycleDuration);
    digitalWrite(STEP, LOW); 
    delayMicroseconds(halfCycleDuration);
  }
  return;
}

void FocuserStepper::SelectMicrostepMode(int microstep)
{
  if(microstep > microstepping)
    microstep = microstepping;
  switch(motorDriver)
  {
    case DRIVERA4988:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=1;
           ms2=1;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
    case DRIVERDRV8825:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=0;
           ms2=1;
           ms3=0;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=0;
           ms2=0;
           ms3=1;
      break;
      case 32:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
    case DRIVERSTSPIN220:
      digitalWrite(STDBY, HIGH);
      switch(microstep)
      {
        case 1:
          ms1=0;
          ms2=0;
          tempdir=0;
          tempstep=0;
        break;
        case 32:
          ms1=0;
          ms2=1;
          tempdir=0;
          tempstep=0;
        break;
        case 128:
           ms1=1;
           ms2=0;
           tempdir=0;
           tempstep=0;
        break;
        case 256:
           ms1=1;
           ms2=1;
           tempdir=0;
           tempstep=0;
        break;
        case 4:
           ms1=0;
           ms2=1;
           tempdir=1;
           tempstep=0;
        break;
        case 64:
           ms1=1;
           ms2=1;
           tempdir=1;
           tempstep=0;
        break;
        case 2:
           ms1=1;
           ms2=0;
           tempdir=0;
           tempstep=1;
        break;
        case 8:
           ms1=1;
           ms2=1;
           tempdir=0;
           tempstep=1;
        break;
        case 16:
           ms1=1;
           ms2=1;
           tempdir=1;
           tempstep=1;
        break;
      }
      digitalWrite(M1, ms1);
      digitalWrite(M2, ms2);
      digitalWrite(DIR, tempdir);
      digitalWrite(STEP, tempstep);  
      digitalWrite(STDBY, LOW);
      delay(100);
    break;
    case DRIVERSTSPIN820:
      switch(microstep)
      {
      case 1:
           ms1=0;
           ms2=0;
           ms3=0;
      break;
      case 2:
           ms1=1;
           ms2=0;
           ms3=0;
      break;
      case 4:
           ms1=0;
           ms2=1;
           ms3=0;
      break;
      case 8:
           ms1=1;
           ms2=1;
           ms3=0;
      break;
      case 16:
           ms1=0;
           ms2=0;
           ms3=1;
      break;
      case 32:
           ms1=1;
           ms2=0;
           ms3=1;
      break;
      case 128:
           ms1=0;
           ms2=1;
           ms3=1;
      break;
      case 256:
           ms1=1;
           ms2=1;
           ms3=1;
      break;
    }
    digitalWrite(M1, ms1);
    digitalWrite(M2, ms2);
    digitalWrite(M3, ms3);
    break;
  }
  maxStepsAbsolute = FOCUSERTRAVEL * 360 * microstep / MOTORRESOLUTION / FOCUSERTRAVELPER360 * GEARRATIO;
  maxStepsAbsolute -= maxStepsAbsolute*SAFETYMAXLIMDECREASEPERCENT/100; // decrease value
  maxSteps = maxStepsAbsolute;
}

void FocuserStepper::ValidateMicrostepMode()
{
  if(microstepping<1)
  {
    Serial2.println(F(" Microstep value can not be negative! Microstep mode will be set to full step."));
    microstepping=1;
  }
  switch(motorDriver)
  {
    case DRIVERSTSPIN820:
    if(microstepping==64)
    {
      Serial2.println(F(" Wrong microstepping option for STSPIN820 driver! Microstep mode will be set to 1/32 step."));
      microstepping=32;
    }
    break;
    case DRIVERDRV8825:
    if(microstepping>32)
    {
      Serial2.println(F(" Wrong microstepping option for STSPIN820 driver!Microstep mode will be set to 1/32 step."));
      microstepping=32;
    }
    break;
    case DRIVERA4988:
    if(microstepping>16)
    {
      Serial2.println(F(" Wrong microstepping option for STSPIN820 driver!Microstep mode will be set to 1/16 step."));
      microstepping=16;
    }
    break;
  }
  switch(microstepping)
  {
    case 1:
      Serial2.println(F(" Microstepping 1/1 step selected"));
    break;
    case 2:
      Serial2.println(F(" Microstepping 1/2 step selected"));
    break;
    case 4:
      Serial2.println(F(" Microstepping 1/4 step selected"));
    break;
    case 8:
      Serial2.println(F(" Microstepping 1/8 step selected"));
    break;
    case 16:
      Serial2.println(F(" Microstepping 1/16 step selected"));
    break;
    case 32:
      Serial2.println(F(" Microstepping 1/32 step selected"));
    break;
    case 64:
      Serial2.println(F(" Microstepping 1/64 step selected"));
    break;
    case 128:
      Serial2.println(F(" Microstepping 1/128 step selected"));
    break;
    case 256:
      Serial2.println(F(" Microstepping 1/256 step selected"));
    break;
    default:
      Serial2.println(F(" Wrong microstepping option! Microstep mode will be set to full step."));
      microstepping=1;
    break;
  }
}

void FocuserStepper::EnableMotor(bool value)
{
  if(value)
  {
    digitalWrite(STDBY, HIGH);
    motorEnable=true;
  }
  else
  {
    digitalWrite(STDBY, LOW);
    motorEnable=false;
  }
}

void FocuserStepper::SetMotorDirection(int direction)
{
  motorDirection = direction;

  if(coilConfiguration <0)
  {
    if( direction > 0 )
      digitalWrite(DIR, motorDirection);
    else
      if(direction < 0)
        digitalWrite(DIR, 0);
  }
  if(coilConfiguration > 0 )
  {
    if( direction > 0 )
      digitalWrite(DIR, 0);
    else
      if(direction < 0)
        digitalWrite(DIR, 1);
  }
}

void FocuserStepper::SetFocuserSpeed(int index)
{
  switch(index)
  {
    case 1:
      halfCycleDuration = 80000/GEARRATIO;
      stepRate = 1;
    break;

    case 2:
      halfCycleDuration = 40000/GEARRATIO;
      stepRate = 1;
    break;

    case 3:
      halfCycleDuration = 20000/GEARRATIO;
      stepRate = 1;
    break;

    case 4:
      halfCycleDuration = 10000/GEARRATIO;
      stepRate = 2;
    break;

    case 5:
      halfCycleDuration = 5000/GEARRATIO;
      stepRate = 4;
    break;

    case 6:
      halfCycleDuration = 2500/GEARRATIO;
      stepRate = 8;
    break;

    case 7:
      halfCycleDuration = 1250/GEARRATIO;
      stepRate = 16;
    break;

    case 8:
      halfCycleDuration = 625/GEARRATIO;
      stepRate = 32;
    break;

    case 9:
      halfCycleDuration = 315/GEARRATIO;
      stepRate = 64;
    break;

    case 10:
      halfCycleDuration = 160/GEARRATIO;
      stepRate = 128;
    break;

    case 11:
      halfCycleDuration = 80/GEARRATIO;
      stepRate = 256;
    break;

    case 12:
      halfCycleDuration = 40/GEARRATIO;
      stepRate = 512;
    break;

    case 13:
      halfCycleDuration = 20/GEARRATIO;
      stepRate = 1024;
    break;

    case 14:
      halfCycleDuration = 10/GEARRATIO;
      stepRate = 2048;
    break;

    case 15:
      halfCycleDuration = 5/GEARRATIO;
      stepRate = 10000;
    break;
  }
  speedIndex = index;
}

void FocuserStepper::CorrelateSpeed(unsigned long numberOfSteps)
{
  if(numberOfSteps > maxSteps/8)
    {
      speedIndex = optimalSpeed;
      SetFocuserSpeed(speedIndex);
      return;
    }
    unsigned long stepTreshold =  maxSteps / 32;
    for(int i=0; i<10; i++)
    {
      if(numberOfSteps > stepTreshold)
      {
        speedIndex = optimalSpeed-1-i;
        SetFocuserSpeed(speedIndex);
        return;
      }
        stepTreshold/=2;
    }
    speedIndex = 1;
    SetFocuserSpeed(1);
}
