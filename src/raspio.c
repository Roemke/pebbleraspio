#include <pebble.h>
#include "control.h"
#include "communication.h"
#include "globals.h"

bool accel = false;
bool vu = true; // vu control on
bool raspio = true; //raspberry control on

int device; //not

void handle_init(void) {
	if (persist_exists(KEY_ACCEL))
		accel = persist_read_bool(KEY_ACCEL);
	else
		accel = false;
	if (persist_exists(KEY_VU))
		vu = persist_read_bool(KEY_VU);
	else
		vu=false;
	if (persist_exists(KEY_RASPIO))
		raspio = persist_read_bool(KEY_RASPIO);
	else
		raspio = false;
  //intialisieren
	raspiFullStatus = string_to_array(0,0,RFSZahl,"?| | ");
	generateStationMenu("...waiting...");
	if (raspio)
		device = RaspiRadio;
	else if (vu)
		device = Vu;
	else
		device = KEY_NOTSET; //bei Installation
	/*
	 * es scheint, dass bei einem einfachen install der javascript-teil seine daten behält
	 * der c-teil die daten aber vergisst, muesste mir also die Daten abholen?
	 */
	APP_LOG(APP_LOG_LEVEL_DEBUG, "start with device %d",device);
	if (device == KEY_NOTSET)
	{
	}
	initCommunication();
	show_control();//creates s_window and layers
    //bind_clicks();

}

void handle_deinit(void) {
  //text_layer_destroy(text_layer);
  //window_destroy(my_window);
	hide_control();
	freeArray(stationList,numberOfStations);
	free(stationList);
	freeArray(raspiFullStatus,RFSZahl);
	free(raspiFullStatus);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
