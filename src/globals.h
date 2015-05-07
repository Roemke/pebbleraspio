//codes needed here and in c-programm
//seems to be neccessary but not nice
//codes for the vuplus
#define KEY_POWER 116
#define KEY_VOLUP  115
#define KEY_VOLDOWN  114
#define KEY_ARRUP  103
#define KEY_ARRDOWN  108
#define KEY_OK  352
#define KEY_INFO  358
#define KEY_EXIT  1
//codes for communication
#define KEY_CONTROL  1000
#define KEY_IP  1001 // IP address of satellite receiver, not really needed
#define KEY_ACCEL  1002	 //use acceleration sensor for control


extern bool accel; //type bool is defined in pebble.h
enum Devices
{
  Vu 			= 0,
  RaspiRadio 	= 1
};

extern uint8_t device;
