
#include <pebble.h>
#include "control.h"
#include "communication.h"
#include "globals.h" //bool defined in pebble, so it must be here

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_pngupdown;
static GBitmap *s_res_pngpower;
static GBitmap *s_res_pngvolume;
static GFont s_res_roboto_condensed_21;
static ActionBarLayer *s_actionbarlayer_1;
static TextLayer *tl_1Arrow;
static TextLayer *tl_3Vol;
static TextLayer *tl_2Power;
static TextLayer *tl_Ident;
//------------------------------------
static char * textPower = "Power";
static char * textVol = "Volume";
static char * textArrow = "Program";
static char * textUp = "Up";
static char * textDown = "Down";
static char * textOK = "OK";
static char * textIdVu="V\nu\n+";
static char * textIdRaspi="R\na\ns\nP\ni\no";
//------------------------------

uint8_t device = 0; //start with the vu+

enum Modes {
  none=0, 
  arrow=1, 
  volume=2 
};
enum AccelModes
{
  accNone 	= 0,
  accUp 	= 1,
  accDown 	= 2
};


static uint8_t mode = none;
static uint8_t accMode = accNone;
static bool accSubscribed = false;
AppTimer * timerScroll = 0;


//window functions
static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  //window_set_fullscreen(s_window, false); //old
  
  s_res_pngupdown = gbitmap_create_with_resource(RESOURCE_ID_pngUpDown);
  s_res_pngpower = gbitmap_create_with_resource(RESOURCE_ID_pngPower);
  s_res_pngvolume = gbitmap_create_with_resource(RESOURCE_ID_pngVolume);
  s_res_roboto_condensed_21 = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
  // s_actionbarlayer_1
  s_actionbarlayer_1 = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer_1, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer_1, GColorWhite);
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_UP, s_res_pngupdown);
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_SELECT, s_res_pngpower);
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_DOWN, s_res_pngvolume);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer_1);
  
  // tl_1Arrow
  tl_1Arrow = text_layer_create(GRect(8, 13, 100, 28));
  text_layer_set_background_color(tl_1Arrow, GColorClear);
  text_layer_set_text_color(tl_1Arrow, GColorWhite);
  text_layer_set_text(tl_1Arrow, textArrow);
  text_layer_set_text_alignment(tl_1Arrow, GTextAlignmentRight);
  text_layer_set_font(tl_1Arrow, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_1Arrow);
  
  // tl_3Vol
  tl_3Vol = text_layer_create(GRect(8, 109, 100, 28));
  text_layer_set_background_color(tl_3Vol, GColorClear);
  text_layer_set_text_color(tl_3Vol, GColorWhite);
  text_layer_set_text(tl_3Vol, textVol);
  text_layer_set_text_alignment(tl_3Vol, GTextAlignmentRight);
  text_layer_set_font(tl_3Vol, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_3Vol);
  
  // tl_2Power
  tl_2Power = text_layer_create(GRect(8, 62, 100, 28));
  text_layer_set_background_color(tl_2Power, GColorClear);
  text_layer_set_text_color(tl_2Power, GColorWhite);
  text_layer_set_text(tl_2Power, textPower);
  text_layer_set_text_alignment(tl_2Power, GTextAlignmentRight);
  text_layer_set_font(tl_2Power, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_2Power);

  // tl_Ident
  tl_Ident = text_layer_create(GRect(4, 7, 25, 155));
  text_layer_set_background_color(tl_Ident, GColorClear);
  text_layer_set_text_color(tl_Ident, GColorWhite);
  text_layer_set_text(tl_Ident, textIdVu);
  text_layer_set_text_alignment(tl_Ident, GTextAlignmentCenter);
  text_layer_set_font(tl_Ident, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_Ident);

}

static void destroy_ui(void) {
  window_destroy(s_window);
  action_bar_layer_destroy(s_actionbarlayer_1);
  text_layer_destroy(tl_1Arrow);
  text_layer_destroy(tl_3Vol);
  text_layer_destroy(tl_2Power);
  gbitmap_destroy(s_res_pngupdown);
  gbitmap_destroy(s_res_pngpower);
  gbitmap_destroy(s_res_pngvolume);
}
// END AUTO-GENERATED UI CODE

static void resetText()
{
  text_layer_set_text(tl_1Arrow,textArrow);
  text_layer_set_text(tl_2Power,textPower);
  text_layer_set_text(tl_3Vol,textVol);
}
static void setAltText(char * middle)
{
  text_layer_set_text(tl_2Power,middle);
  text_layer_set_text(tl_1Arrow,textUp);
  text_layer_set_text(tl_3Vol,textDown);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
  //APP_LOG(APP_LOG_LEVEL_INFO,"Done is %i warnown is %i minutes is %i ",done ? 1 : 0, 
  //                       warnown ? 1 : 0, minutes);
  //APP_LOG(APP_LOG_LEVEL_INFO,"Destroy window");
}

static void timerScrollRun()
{
	if (accMode == accUp && mode == volume)
		sendVolUp();
	else if (accMode == accDown && mode == volume)
		sendVolDown();
	else if (accMode == accUp && mode == arrow)
		sendArrowUp();
	else if (accMode == accDown && mode==arrow)
		sendArrowDown();

	timerScroll = 0;
	timerScroll = app_timer_register(500, timerScrollRun, 0); //call it again
}


//handle accelarator data
static void data_handler(AccelData *data, uint32_t num_samples) {
  //static char s_buffer[128];
  for (int i = 0; i < 3; ++i)
  {
	  //if (data[i].y > 1000 && data[i].z > -500 && accMode != accDown)
	  if (data[i].y > 500 && data[i].z > -700 && accMode != accDown)
	  {
		  //snprintf(s_buffer, sizeof(s_buffer),
			//  "%d %d,%d,%d",i, data[i].x, data[i].y, data[i].z);
		  //APP_LOG(APP_LOG_LEVEL_INFO,"should sitch up %s",s_buffer);
		  accMode = accDown;
		  timerScrollRun();
		  //I think so, otherwise the signals to the phone come to fast
		  break;
	  }
	  //else if (data[i].y < -900 && data[i].z > -500 && accMode != accUp)
	  else if (data[i].y < -500 && data[i].z > -700 && accMode != accUp)
	  {
		  accMode = accUp;
		  //snprintf(s_buffer, sizeof(s_buffer),
			//  "%d %d,%d,%d",i, data[i].x, data[i].y, data[i].z);
		  //APP_LOG(APP_LOG_LEVEL_INFO,"should switch on %s",s_buffer);
		  timerScrollRun();
		  break;
	  }
	  else if (data[i].y > -300  && data[i].y < 300 && data[i].z < -700 && (accMode == accUp || accMode ==accDown)	 )
	  {
		  accMode = accNone;
		  if (timerScroll)
			  app_timer_cancel(timerScroll);
		  timerScroll = 0;
		  //snprintf(s_buffer, sizeof(s_buffer),
			//  "%d %d,%d,%d",i, data[i].x, data[i].y, data[i].z);
		  //APP_LOG(APP_LOG_LEVEL_INFO,"should switch off up/down %s",s_buffer);
		  break;
	  }
  }
}
static void switchOnAccel()
{
	int num_samples = 3;
	accSubscribed = true;
    accel_data_service_subscribe(num_samples, data_handler);
    // Choose update rate, 25 is standard
    accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
}
static void switchOffAccel()
{
	//APP_LOG(APP_LOG_LEVEL_INFO,"accMode: %d   -  accSubscribe: %d timerscroll: %p",
	  //  accMode, accSubscribed, timerScroll); 
	if (accSubscribed)
		accel_data_service_unsubscribe();//data recording not longer needed
	if (timerScroll)
		app_timer_cancel(timerScroll);
	timerScroll = 0;
	accMode = none; //no mode
	accSubscribed = false;
}

//back button left
static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (mode)
	{
		case none:
			//we have to leave the app 
			hide_control();//could crash will see, no works
			break;
		case volume:
			mode=none;
			switchOffAccel();
			break;
		case arrow:
			mode=none;
			switchOffAccel();
			sendExit();
			break;
	}
	resetText();
}

//select long click - previous
//switch controlled device - we have only to in the moment, so we
//switch between them
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
	switch (device)
	{
	case Vu:
		device = RaspiRadio;
		text_layer_set_text(tl_Ident, textIdRaspi);
		break;
	case RaspiRadio:
		device = Vu;
		text_layer_set_text(tl_Ident, textIdVu);
		break;
	}
}
//volume
static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (mode)
	{
		case none:
			setAltText(textVol);
			mode = volume;
			if (accel)
				switchOnAccel();
			sendVolDown();
			break;
	  case volume:
			sendVolDown();
			break;
	  case arrow:
			sendArrowDown();
			break;
	}
}
//arrowramm / progs
static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //... called on single click ...
  //Window *window = (Window *)context;	
  switch (mode)
	{
  	  case none:
			setAltText(textOK);
			mode = arrow;
			sendArrowDown();
			sendArrowUp(); //back to original position 
			if (accel)
				switchOnAccel();
			break;
	  case volume:
			sendVolUp();
			break;
	  case arrow:

			sendArrowUp();
			break;
	}
}
//misc
static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //... called on single click ...
  //Window *window = (Window *)context;
	switch ( mode)
	{
		case volume:
			mode = none; //just go back
			switchOffAccel();
			resetText();
			break;
		case none:
			switchOffAccel();
			sendPower();
			break;
		case arrow:
			switchOffAccel();
			sendOK();
			break;
	}
}

static void config_provider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler); //Back

  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 300, down_single_click_handler);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler); //Volume
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 300, up_single_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler); //Power

  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);//Arrows (Program)
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
  //window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 1000, select_single_click_handler);

  // multi click config:
  //window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0, true, select_multi_click_handler);

  // long click config:
  //window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, select_long_click_release_handler);
}

void show_control(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  //APP_LOG(APP_LOG_LEVEL_INFO,"pushed on stack");

}

void hide_control(void) {
  window_stack_remove(s_window, true);
}

void bind_clicks(void)
{
	  window_set_click_config_provider(s_window, (ClickConfigProvider) config_provider);
}
