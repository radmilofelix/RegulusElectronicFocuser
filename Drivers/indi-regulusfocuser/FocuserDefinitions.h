
///////    Modbus    ////////////////////////////////////////////////
#define REGCOMMANDFROMDRIVER		0
#define REGRESPONSETODRIVER			1
#define REGREQUESTEDPOSITIONLO      2
#define REGREQUESTEDPOSITIONHI      3
#define REGREQUESTEDSPEED           4
#define REGFLASHPOSITIONLO          5
#define REGFLASHPOSITIONHI          6
#define REGSTEPPOSITIONLO			7
#define REGSTEPPOSITIONHI   		8
#define REGMAXSTEPSLO               9
#define REGMAXSTEPSHI               10
#define REGMOTORENABLED             11 // Motor Enabled/Disabled
#define REGTARGETSTEPPOSITIONLO     12
#define REGTARGETSTEPPOSITIONHI     13
#define REGFOCUSERSPEED             14
#define REGDIRECTION                15 // 1 = Outward, 2 = Inward
#define REGREMOTECONTROL            16 // 1 = Enable, 0 = Disable
#define REGFAULT                 	17 // 1 = Faulty, 0 = OK
#define REGBACKLASHLO               18
#define REGBACKLASHHI               19
// -------------------------------------------------------------------
#define NUMBEROFREGISTERS           20
//////////////////////////////////////////////////////////////////////

///////    Commands    ///////////////////////////////////////////////
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
#define CMDREMOTEENABLE	    212
#define CMDREMOTEDISABLE    213
#define CMDSETPOSITION      214
//////////////////////////////////////////////////////////////////////
