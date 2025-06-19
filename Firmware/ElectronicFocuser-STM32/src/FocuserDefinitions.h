#ifndef FOCUSERDEFINITIONS___H
#define FOCUSERDEFINITIONS___H

//#define DEBUG

#define PROGRAMVERSION 1.0

///////    Modbus    ////////////////////////////////////////////////
#define REGCOMMANDFROMPI			0
#define REGREQUESTEDPOSITIONLO      1
#define REGREQUESTEDPOSITIONHI      2
#define REGREQUESTEDSPEED           3
#define REGRESPONSETOPI				4
#define REGSTEPPOSITIONLO			5
#define REGSTEPPOSITIONHI   		6
#define REGMAXSTEPSLO               7
#define REGMAXSTEPSHI               8
#define REGMOTORENABLED             9 // Motor Enabled/Disabled
#define REGTARGETSTEPPOSITIONLO     10
#define REGTARGETSTEPPOSITIONHI     11
#define REGFOCUSERSPEED             12
#define REGDIRECTION                13 // 1 = Outward, 2 = Inward
#define REGREMOTECONTROL            14 // 1 = Enable, 0 = Disable
#define REGFAULT                    15 // 1 = Faulty, 0 = OK
// -------------------------------------------------------------------
#define NUMBEROFREGISTERS           16
//////////////////////////////////////////////////////////////////////

///////    Commands    ///////////////////////////////////////////////
#define CMDENABLEMOTOR  200
#define CMDDISABLEMOTOR 201
#define CMDSETSPEED     202				
#define CMDGOTOPOSITION 203
#define CMDGOTOZERO     204
#define CMDGOTOMAX      205
#define CMDINIT         206
#define TICKPLUS        207
#define TICKMINUS       208
#define CMDGOTORELATIVE 209
#define CMDEXTEND		210
#define CMDRETRACT		211
#define CMDREMOTEENABLE	212
#define CMDREMOTEDISABLE 213
//////////////////////////////////////////////////////////////////////

#endif