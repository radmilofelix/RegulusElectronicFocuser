#ifndef FOCUSERDEFINITIONS___H
#define FOCUSERDEFINITIONS___H

//#define DEBUG

#define PROGRAMVERSION 1.0

///////    Modbus    ////////////////////////////////////////////////
#define REGCOMMANDFROMDRIVER		0 //data from driver
#define REGRESPONSETODRIVER			1 //init,
#define REGREQUESTEDPOSITIONLO      2 //data from driver
#define REGREQUESTEDPOSITIONHI      3 //data from driver
#define REGREQUESTEDSPEED           4 //data from driver
#define REGFLASHPOSITIONLO          5 //data from driver
#define REGFLASHPOSITIONHI          6 //data from driver
#define REGSTEPPOSITIONLO			7 //init, polled
#define REGSTEPPOSITIONHI   		8 //init, polled
#define REGMAXSTEPSLO               9 //init, polled
#define REGMAXSTEPSHI               10 //init, polled
#define REGMOTORENABLED             11 //init, Motor Enabled/Disabled
#define REGTARGETSTEPPOSITIONLO     12 //init, polled
#define REGTARGETSTEPPOSITIONHI     13 //init, polled
#define REGFOCUSERSPEED             14 //init, polled
#define REGDIRECTION                15 //polled, 1 = Outward, 2 = Inward ????
#define REGREMOTECONTROL            16 //polled, 1 = Enable, 0 = Disable
#define REGFAULT                    17 //polled, 1 = Faulty, 0 = OK
#define REGBACKLASHLO               18 //init, polled
#define REGBACKLASHHI               19 //init, polled
// -------------------------------------------------------------------
#define NUMBEROFREGISTERS           20
//////////////////////////////////////////////////////////////////////

///////    Commands    ///////////////////////////////////////////////
#define CMDSPEED1       1
#define CMDSPEED2       2
#define CMDSPEED3       3
#define CMDSPEED4       4
#define CMDSPEED5       5
#define CMDSPEED6       6
#define CMDSPEED7       7
#define CMDSPEED8       8
#define CMDSPEED9       9
#define CMDSPEED10      10
#define CMDSPEED11      11
#define CMDSPEED12      12
#define CMDSPEED13      13
#define CMDSPEED14      14
#define CMDSPEED15      15

#define CMDENABLEMOTOR      200
#define CMDDISABLEMOTOR     201
#define CMDSETSPEED         202				
#define CMDGOTOPOSITION     203
#define CMDGOTOZERO         204
#define CMDGOTOMAX          205
#define CMDINIT             206
#define TICKPLUS            207
#define TICKMINUS           208
#define CMDGOTORELATIVE     209
#define CMDEXTEND		    210
#define CMDRETRACT		    211
#define CMDREMOTEENABLE     212
#define CMDREMOTEDISABLE    213
#define CMDSETPOSITION      214
//////////////////////////////////////////////////////////////////////

#endif