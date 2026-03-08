#ifndef FOCUSERSTEPPER___H
#define FOCUSERSTEPPER___H

#include <Arduino.h>

// Travel distance: 17.5mm
// Travel distance per big wheel revolution: 13mm
// Steps per revolution (1/256 microstepping): 102400
// Total steps (1/256 microstepping): 138240
// Travel per step (1/256 microstepping): 126.953125nm

//#define FOCUSERDEBUG

#define EN  PB4     
#define M1  PB6 // A4988 MS1; DRV8825 M0; STSPIN220 MODE1; STSPIN820 M1       
#define M2  PB7 // A4988 MS2; DRV8825 M1; STSPIN220 MODE2; STSPIN820 M2 
#define M3  PB8 // A4988 MS3; DRV8825 M2; STSPIN220 (1); STSPIN820 M3 
#define STDBY  PB5
#define STEP  PB1
#define DIR  PB3
#define RESET PB5
#define TX2PIN PA2

#define DRIVERA4988 1
#define DRIVERDRV8825 2
#define DRIVERSTSPIN220 3
#define DRIVERSTSPIN820 4
#define LIMITSWITCHPIN PB12
#define LIMITSWITCHNORMALLYCLOSED
#define SAFETYMAXLIMDECREASEPERCENT 5
#define MAXGEAREDBACKLASH 100000

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////  Enable one of the focuser models
//#define SCT_MCT
//#define PHOTON_254
#define RACK_PINION_LO_PROFILE
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SCT_MCT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Crayford 2 " Focuser for SCT & MCT - parameters
 #define FOCUSERTRAVEL 17 // mm
 #define FOCUSERTRAVELPER360 13 // mm
 #define GEARRATIO 1 // Direct drive
 #define MOTORRESOLUTION 1.8 // degrees per step
 #define MAXMICROSTEPPING 256
 #define GEAREDTYPE false
// UnscrewToRetract
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

#ifdef PHOTON_254
////// Crayford 2 " Focuser for Newt. Photon 254 f:4 - parameters
 #define FOCUSERTRAVEL 50 // mm
 #define FOCUSERTRAVELPER360 13 // mm
 #define GEARRATIO 1 // Direct drive
 #define MOTORRESOLUTION 1.8 // degrees per step
 #define MAXMICROSTEPPING 256
 #define GEAREDTYPE false
// UnscrewToRetract
#endif

#ifdef RACK_PINION_LO_PROFILE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Rack & Pinion low profile 2" focuser for Newtonians - parameters
 #define FOCUSERTRAVEL  23 // mm
 //#define FOCUSERTRAVEL  23 // mm
 #define FOCUSERTRAVELPER360 23 // mm
 #define GEARRATIO 100 // 
 #define MOTORRESOLUTION 1.8 // degrees per step
 #define MAXMICROSTEPPING 256
 #define GEAREDTYPE true
 // UnscrewToRetract
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocuserStepper
{
    public:
    FocuserStepper();
    ~FocuserStepper();
	int PulseStep (uint32_t numberOfSteps, int motionDirection, int duration);
	void PulseStepToTarget ();
	void RelativePulseStepToTarget(uint32_t relativeSteps);
	int ToneStep (uint32_t numberOfSteps, int motionDirection, uint32_t frequency);
	void SelectMicrostepMode(int microstep);
	void ValidateMicrostepMode();
	void EnableMotor(bool value);
	void SetMotorDirection(int direction);
	void SetFocuserSpeed(int index);
	void CorrelateSpeed(uint32_t numberOfSteps);
	
	int motorDirection; // -1 = retract; 1 = extend
	int coilConfiguration; // 1 = normal; 2 = reversed
	bool motorEnable;
	uint32_t steps;
	uint32_t freq;
	int halfCycleDuration; // step half cycle duration in microseconds
	int microstepping;
	int ms1;
	int ms2;
	int ms3;
	int tempdir,tempstep;
	int motorDriver;

	int speedIndex; // index for focuser speeds
	int optimalSpeed; // optimal maximal speed to be used
	long stepPosition; // focuser position in steps
	long stepTarget; // target position for focuser
	long stepRate; // number of step per encoder tick
    bool gearedType;
    int backlash;
	uint32_t maxSteps; // maximal number of steps in use
	uint32_t maxStepsAbsolute; // maximum steps calculated from FOCUSERTRAVEL, FOCUSERTRAVELPER360, MOTORRESOLUTION, MAXMICROSTEPPING and GEARRATIO values
};

#endif
