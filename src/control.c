
#include <pebble.h>
#include "control.h"
#include "communication.h"
#include "globals.h" //bool defined in pebble, so it must be here
#include "control.h"

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GBitmap *s_res_pngStation;
static GBitmap *s_res_pngPower;
static GBitmap *s_res_pngVolume;
static GBitmap *s_res_pngBigIcon;
static GFont s_res_roboto_condensed_21;
static ActionBarLayer *s_actionbarlayer_1;
static TextLayer *tl_1Program;
static TextLayer *tl_3Vol;
static TextLayer *tl_2Power;
static TextLayer *tl_Ident; //vu or raspio
static TextLayer *tl_Station; //Station for Raspberry - empty on vu +
static TextLayer *tl_Info ; // Info field for raspio - empty on vu + because you will have a screen :-)
static TextLayer *tl_Vol ;
static BitmapLayer *bl_Icon;
//------------------------------------
static char * textVol = "Volume";
static char * textProgram = "Program";
static char * textPower;
static char * textId;
static char * textInfo;
static char * textStation;
static char * textEmpty="";

char textVolRaspi[] = "---"; //3 zeichen
char  * raspiFullStatus[RFSZahl]={0}; //need to be on heap, RFSZahl in c nur per define
//------------------------------

uint8_t device = RaspiRadio; //start with the Radio need more often / koennte man auch configurierbar machen

#if defined(PBL_COLOR)
 #define ColorBabyBlue  GColorBabyBlueEyes
#elif defined(PBL_BW)
 #define ColorBabyBlue  GColorWhite
#endif

enum Modes {
  none=0, 
  program=1,
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

static void setDeviceSpecifics()
{
	static char * textPowerVu = "Power";
	static char * textPowerRaspi = " ";
	static char * textIdVu="V\nu\n+";
	static char * textIdRaspi="R\na\ns\nP\ni\no";
	static char * textInfoVu = "";
	static char * textStationVu = "";
	static char * textStationRaspi = "?";
	static char * textInfoRaspi = "";

	switch (device)
	{
	case  RaspiRadio:
		textPower = textPowerRaspi;
		textId    = textIdRaspi;
		textInfo  = textInfoRaspi;
		textStation = textStationRaspi;
		break;
	case Vu :
		textPower 	= 	textPowerVu;
		textId		= 	textIdVu;
		textInfo    =	textInfoVu;
		textStation = 	textStationVu;
		break;
	}
}

void setLayersVisibility(void)
{
	  //Vu -> device == 0 Raspberry device == 1
	  // modes none=0, program=1,volume=2
	 //hidden raspberry layers fuer vu auf jeden Fall nicht sichtbar
	  layer_set_hidden((Layer *) tl_Info,!device || (mode != none));
	  layer_set_hidden((Layer *) tl_Station,!device || (mode != none));
	  layer_set_hidden((Layer *) tl_Vol, !device || (mode != volume) );

	  //hidden vu layers
	  layer_set_hidden((Layer *) tl_1Program,  device || (mode != none));
	  layer_set_hidden((Layer *) tl_2Power,device || (mode != none));
	  layer_set_hidden((Layer *) tl_3Vol,device || (mode != none));
}
//window functions
static void initialise_ui(void) {
  setDeviceSpecifics();

  s_window = window_create();

  window_set_background_color(s_window, GColorBlack);
  //window_set_fullscreen(s_window, false); //old
  
  s_res_pngStation = gbitmap_create_with_resource(RESOURCE_ID_pngStation);
  s_res_pngPower = gbitmap_create_with_resource(RESOURCE_ID_pngPower);
  s_res_pngVolume = gbitmap_create_with_resource(RESOURCE_ID_pngVolume);
  s_res_pngBigIcon = 0;

  s_res_roboto_condensed_21 = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
  // s_actionbarlayer_1
  s_actionbarlayer_1 = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer_1, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer_1,ColorBabyBlue );
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_UP, s_res_pngStation);
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_SELECT, s_res_pngPower);
  action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_DOWN, s_res_pngVolume);

  //no animation on old pebble
  #if defined (PBL_COLOR)
  	  action_bar_layer_set_icon_press_animation(s_actionbarlayer_1,BUTTON_ID_UP,ActionBarLayerIconPressAnimationMoveUp);
  	  action_bar_layer_set_icon_press_animation(s_actionbarlayer_1,BUTTON_ID_DOWN,ActionBarLayerIconPressAnimationMoveDown);
  #endif
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer_1);
  

  // tl_1Program
  tl_1Program = text_layer_create(GRect(8, 14, 100, 28));
  text_layer_set_background_color(tl_1Program, GColorClear);
  text_layer_set_text_color(tl_1Program, GColorWhite);
  text_layer_set_text(tl_1Program, textProgram);
  text_layer_set_text_alignment(tl_1Program, GTextAlignmentRight);
  text_layer_set_font(tl_1Program, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_1Program);
  
  // tl_2Power
  tl_2Power = text_layer_create(GRect(8, 69, 100, 28));
  text_layer_set_background_color(tl_2Power, GColorClear);
  text_layer_set_text_color(tl_2Power, GColorWhite);
  text_layer_set_text(tl_2Power, textPower);
  text_layer_set_text_alignment(tl_2Power, GTextAlignmentRight);
  text_layer_set_font(tl_2Power, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_2Power);

  // tl_3Vol
    tl_3Vol = text_layer_create(GRect(8, 124, 100, 28));
    text_layer_set_background_color(tl_3Vol, GColorClear);
    text_layer_set_text_color(tl_3Vol, GColorWhite);
    text_layer_set_text(tl_3Vol, textVol);
    text_layer_set_text_alignment(tl_3Vol, GTextAlignmentRight);
    text_layer_set_font(tl_3Vol, s_res_roboto_condensed_21);
    layer_add_child(window_get_root_layer(s_window), (Layer *)tl_3Vol);

  // tl_Ident
  tl_Ident = text_layer_create(GRect(4, 7, 25, 155));
  text_layer_set_background_color(tl_Ident, GColorClear);
  text_layer_set_text_color(tl_Ident, GColorWhite);
  text_layer_set_text(tl_Ident, textId);
  text_layer_set_text_alignment(tl_Ident, GTextAlignmentCenter);
  text_layer_set_font(tl_Ident, s_res_roboto_condensed_21);
  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_Ident);

  //tl_Station Station of Raspberry pi
  //nehmen wir mal die Auflösung des Windows - sollte eigentlich immer relativ arbeiten,
  //naja, was solls
  int stationHeight = 30;
  tl_Station = text_layer_create(GRect(32, 1, 80, stationHeight ));
  text_layer_set_background_color(tl_Station, GColorClear);
  text_layer_set_text_color(tl_Station, ColorBabyBlue);
  text_layer_set_text_alignment(tl_Station, GTextAlignmentLeft);
  text_layer_set_font(tl_Station, s_res_roboto_condensed_21);
  text_layer_set_text(tl_Station, textStation);

  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_Station);

  //tl_Info Text Layer for information from Raspio
  //nehmen wir mal die Auflösung des Windows - sollte eigentlich immer relativ arbeiten,
  //naja, was solls
  Layer * window_layer = window_get_root_layer(s_window);
  GRect window_bounds = layer_get_bounds(window_layer);
  tl_Info = text_layer_create(GRect(32, stationHeight + 2, 80, window_bounds.size.h-3-stationHeight ));
  text_layer_set_background_color(tl_Info, GColorClear);
  text_layer_set_text_color(tl_Info, GColorWhite);
  text_layer_set_text(tl_Info, textInfo);
  text_layer_set_text_alignment(tl_Info, GTextAlignmentLeft);
  text_layer_set_font(tl_Info, s_res_roboto_condensed_21);

  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_Info);

  //tl_Vol Text Layer Volume
  tl_Vol = text_layer_create(GRect(32, stationHeight + 22, 80, window_bounds.size.h-23-stationHeight ));
  text_layer_set_background_color(tl_Vol, GColorClear);
  text_layer_set_text_color(tl_Vol, ColorBabyBlue);
  text_layer_set_text(tl_Vol, textVolRaspi);
  text_layer_set_text_alignment(tl_Vol, GTextAlignmentCenter);
  text_layer_set_font(tl_Vol, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));

  layer_add_child(window_get_root_layer(s_window), (Layer *)tl_Vol);

  //Bitmaplayer for icon which indicate mode
  bl_Icon = bitmap_layer_create(GRect(32, 1, 80, stationHeight+10 ));
  layer_add_child(window_get_root_layer(s_window),(Layer *) bl_Icon);
  //ueberschreibt noch nicht - layer ist bei GColorClear transparent
  /*
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "s_res_pngStation %p", s_res_pngStation);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "s_res_pngPower %p", s_res_pngPower);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "s_res_pngVolume %p", s_res_pngVolume);
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "s_res_pngBigIcon %p", s_res_pngBigIcon);
  */
  action_bar_layer_set_click_config_provider(s_actionbarlayer_1, (ClickConfigProvider) click_config_provider);

  setLayersVisibility();
  //evtl. Informationen vom Device abholen, nein hier zu früh
}



static void destroy_ui(void) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "call to destroy_ui");
  gbitmap_destroy(s_res_pngStation);
  gbitmap_destroy(s_res_pngPower);
  gbitmap_destroy(s_res_pngVolume);
  if (s_res_pngBigIcon)
	  gbitmap_destroy(s_res_pngBigIcon);
  action_bar_layer_destroy(s_actionbarlayer_1);
  text_layer_destroy(tl_1Program);
  text_layer_destroy(tl_3Vol);
  text_layer_destroy(tl_2Power);
  text_layer_destroy(tl_Ident);
  text_layer_destroy(tl_Info);
  text_layer_destroy(tl_Station);
  text_layer_destroy(tl_Vol);
  bitmap_layer_destroy(bl_Icon);
  window_destroy(s_window);
}
// END AUTO-GENERATED UI CODE

static void resetText()
{
  setDeviceSpecifics(); //depends on device
  text_layer_set_text(tl_1Program,textProgram);
  text_layer_set_text(tl_2Power,textPower);
  text_layer_set_text(tl_3Vol,textVol);
  text_layer_set_text(tl_Ident,textId);
}


void rearrangeGui()
{
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "call to rearrangeGui");
	//Null is the same as clear
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_UP, NULL);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_SELECT, NULL);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_DOWN, NULL);

	//destroy existing bitmaps
	gbitmap_destroy(s_res_pngStation);
	gbitmap_destroy(s_res_pngPower);
	gbitmap_destroy(s_res_pngVolume);

	if (s_res_pngBigIcon)
	  gbitmap_destroy(s_res_pngBigIcon);

	s_res_pngBigIcon = gbitmap_create_with_resource(RESOURCE_ID_pngStation40);

	switch (mode)
	{
	case volume:
		gbitmap_destroy(s_res_pngBigIcon);
		s_res_pngBigIcon = gbitmap_create_with_resource(RESOURCE_ID_pngVolume40);
	case program:
		s_res_pngStation = gbitmap_create_with_resource(RESOURCE_ID_pngUpArrow);
		s_res_pngPower  = gbitmap_create_with_resource(RESOURCE_ID_pngOk);
		s_res_pngVolume = gbitmap_create_with_resource(RESOURCE_ID_pngDownArrow);
		text_layer_set_text(tl_1Program, textEmpty);
		text_layer_set_text(tl_2Power, textEmpty);
		text_layer_set_text(tl_3Vol, textEmpty);
		bitmap_layer_set_bitmap(bl_Icon,s_res_pngBigIcon);
		break;
	case none:
		bitmap_layer_set_bitmap(bl_Icon,0);
		gbitmap_destroy(s_res_pngBigIcon);
		s_res_pngBigIcon = 0;
		s_res_pngStation = gbitmap_create_with_resource(RESOURCE_ID_pngStation);
		s_res_pngPower = gbitmap_create_with_resource(RESOURCE_ID_pngPower);
		s_res_pngVolume = gbitmap_create_with_resource(RESOURCE_ID_pngVolume);
		text_layer_set_text(tl_1Program, textProgram);
		text_layer_set_text(tl_2Power, textPower);
		text_layer_set_text(tl_3Vol, textVol);
		text_layer_set_text(tl_Station,raspiFullStatus[0]);
		text_layer_set_text(tl_Info,raspiFullStatus[1]);
		break;
	}
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_UP, s_res_pngStation);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_SELECT, s_res_pngPower);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_DOWN, s_res_pngVolume);
	setLayersVisibility();
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
	else if (accMode == accUp && mode == program)
		sendProgramUp();
	else if (accMode == accDown && mode==program)
		sendProgramDown();

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
			hide_control();//could crash will see, no works and is neccessary
			//to stop application
			break;
		case volume:
			mode=none;
			switchOffAccel();
			rearrangeGui();
			resetText();
			break;
		case program:
			mode=none;
			switchOffAccel();
			sendExit();
			rearrangeGui();
			resetText();
			break;
	}
}

//select long click - previous
//switch controlled device - we have only to in the moment, so we
//switch between them
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
	if (vu && raspio) //both should be controlled
	{
		switch (device)
		{
		case Vu:
			device = RaspiRadio;
			break;
		case RaspiRadio:
			device = Vu;
			break;
		}
		sendNewDevice();//device is global
		mode = none; //just go back
		rearrangeGui();
		switchOffAccel();
		resetText();
	}
}
//volume
static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (mode)
	{
		case none:
			mode = volume;
			if (accel)
				switchOnAccel();
			if (device == Vu)
				sendVolDown();//to show on vu
			else if (device == RaspiRadio)
				getVolume();
			break;
	  case volume:
			sendVolDown();
			break;
	  case program:
			sendProgramDown();
			break;
	}
	rearrangeGui();
}
//up click
static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //... called on single click ...
  //Window *window = (Window *)context;	
  switch (mode)
	{
  	  case none:
			mode = program;
			sendProgramDown();
			sendProgramUp(); //back to original position
			if (accel)
				switchOnAccel();
			break;
	  case volume:
			sendVolUp();
			break;
	  case program:
			sendProgramUp();
			break;
	}
	rearrangeGui();

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
			if (device == Vu)
				sendPower();
			else if (device == RaspiRadio)
				getFullStatus(); //aktualisieren der Anzeige
			break;
		case program:
			switchOffAccel();
			sendOK();
			break;
	}
	rearrangeGui();

}

//provider fuer die action_bar
static void  click_config_provider(Window * window)
{
	// single click / repeat-on-hold config:
	  window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler); //Back

	  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 300, down_single_click_handler);
	  //window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler); //Volume
	  window_single_repeating_click_subscribe(BUTTON_ID_UP, 300, up_single_click_handler);
	  //window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler); //Power

	  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);//Arrows (Program)
	  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
}
//war ein provider fuer das window, nutze jetzt mal actionbar
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
  freeArray(raspiFullStatus,RFSZahl);
}

void bind_clicks(void)
{
	  window_set_click_config_provider(s_window, (ClickConfigProvider) config_provider);
}
