//codes needed here and in js programs
//seems to be neccessary but not nice
//codes for the vuplus
#define KEY_NOTSET 0
#define KEY_VU 1
#define KEY_RASPIO  2

#define KEY_APPREADY 10

#define KEY_POWER 116
#define KEY_VOLUP  115
#define KEY_VOLDOWN  114
#define KEY_STATIONUP  103
#define KEY_STATIONDOWN  108
#define KEY_OK  352
#define KEY_INFO  358
#define KEY_EXIT  1

//codes for settings
#define KEY_CONTROL  1000
#define KEY_VUIP  1001 // IP address of satellite receiver, not really needed js app knows it
#define KEY_ACCEL  1002	 //use acceleration sensor for control
#define KEY_RASPIP 1005
#define KEY_DEVICE 1006 //send this together with new device KEY_VU or KEY_RASPIO

#define KEY_RASPVOLUME      1100
#define KEY_RASPFULLSTATUS  1101
#define KEY_RASPSTATIONLIST  1102
#define KEY_RASPACTUALSTAION 1103
#define KEY_SWITCHSTATION 1104



//extern ist doch eigentlich der standard wenn nicht static angegeben
extern bool accel; //type bool is defined in pebble.h, acceleration on
extern bool vu; // vu control on
extern bool raspio; //raspberry control on
enum Devices
{
  Vu 			= KEY_VU,
  RaspiRadio 	= KEY_RASPIO,
  NotSet 		= KEY_NOTSET
};

extern int device;
extern char textVolRaspi[];
extern char ** raspiFullStatus;
#define RFSZahl 3
extern int numberOfStations;
extern int actualStation; //0 based number of actual station in list
extern char **stationList;

