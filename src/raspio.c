#include <pebble.h>
#include "control.h"
#include "communication.h"
#include "globals.h"

bool accel = false;

void handle_init(void) {
	if (persist_exists(KEY_ACCEL))
	{
		accel = persist_read_bool(KEY_ACCEL);
	}

	initCommunication();
	show_control();//creates s_window and layers
  bind_clicks(); 

}

void handle_deinit(void) {
  //text_layer_destroy(text_layer);
  //window_destroy(my_window);
	hide_control();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
