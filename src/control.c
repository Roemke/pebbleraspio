
#include <pebble.h>
#include "control.h"
#include "communication.h"
#include "globals.h" //bool defined in pebble, so it must be here
#include "control.h"

/*
 * nehme noch ein window fuer den menulayer dazu, wenn ich den menulayer direkt
 * auf das aktuelle window setze, dann bekomme ich den backevent nicht abgefangen ohne
 * die funktionalität der up un down tasten zunichte zu machen
 * mit window fuer menu layer geht es, kann mich in den unload event einhängen
 *
 * bleibt das Problem: beim scrollen durch die Liste werden die unteren Bereiche nicht hoch gescrollt
 * hatte gelesen: menu_layer_reload_data loest das Problem hatte das in selection changed gesetzt
 * -> fantastischer Absturz mit vergessen der firmware
 * in click event genommen -> es geht, also in redrawStations und alles ist ok
 * */

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
//naja, damit hat es angefangen
static Window *s_window, *menu_window;
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
static MenuLayer * ml_Stations;
//------------------------------------
static char * textVol = "Volume";
static char * textProgram = "Program";
static char * textPower;
static char * textId;
static char * textInfo;
static char * textStation;
static char * textEmpty="";

char textVolRaspi[] = "---"; //3 zeichen
char  ** raspiFullStatus=0; //need to be on heap

static void  click_config_provider(Window * window);
static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) ;

//------------------------------

int actualStation = 0;

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
AppTimer * timerAktualisiereStatus = 0;


static void aktualisiereStatus()
{
	if (device == RaspiRadio)
	{
		getFullStatus();
		 //APP_LOG(APP_LOG_LEVEL_DEBUG, "timer aktualisiere status");
		timerAktualisiereStatus = app_timer_register(5000,aktualisiereStatus,0);
	}
	else
		timerAktualisiereStatus = 0;
}
//4 callbacks for the menu to select a station
static uint16_t getNumberOfStations(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return numberOfStations;
}

static void drawStationRaw (GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data)
{
	//menu_cell_title_draw(ctx, cell_layer, stationList[cell_index->row]);
	menu_cell_basic_draw(ctx, cell_layer, stationList[cell_index->row], NULL, NULL);
}
static int16_t getCellHeight(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 28;
}
/*experimente
static void stationSelectChanged (MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context)
{
	  APP_LOG(APP_LOG_LEVEL_DEBUG, "changed from %d to %d",old_index.row,new_index.row);
	  //menu_layer_reload_data(ml_Stations); fuehrt zum absturz
	  //layer_mark_dirty((Layer *) menu_layer);
}
*/
static void stationSelect(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	actualStation = cell_index->row;
	switchStation(actualStation);
}
//-------------------------------------------

void redrawStationMenu()
{
	//layer_mark_dirty((Layer *) ml_Stations);
	menu_layer_reload_data(ml_Stations);
	menu_layer_set_selected_index(ml_Stations,
									(MenuIndex) {.section=0,.row=actualStation},
									MenuRowAlignCenter,true);
}
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
	  layer_set_hidden((Layer *) tl_Info, (device == Vu ) || (mode != none));
	  layer_set_hidden((Layer *) tl_Station,(device == Vu) || (mode != none));
	  layer_set_hidden((Layer *) tl_Vol, (device == Vu)|| (mode != volume) );

	  // Bind the menu layer's click config provider to the window for interactivity
	  if (device == RaspiRadio && mode == program)
	  {
		  window_stack_push(menu_window, true); //wird sichtbar
	  }

	  //vu layers hidden for raspberry
	  layer_set_hidden((Layer *) tl_1Program,  (device == RaspiRadio) || (mode != none));
	  layer_set_hidden((Layer *) tl_2Power,(device == RaspiRadio) || (mode != none));
	  layer_set_hidden((Layer *) tl_3Vol,(device == RaspiRadio) || (mode != none));
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
  action_bar_layer_set_click_config_provider(s_actionbarlayer_1, (ClickConfigProvider) click_config_provider);
  

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

  //stations menu - simplemenu geht nicht alles const
  menu_window = window_create();
  ml_Stations = menu_layer_create(window_bounds); //fullscreen
  //menu erzeugt die Daten via callback
  menu_layer_set_callbacks(ml_Stations, 0, //0 : no contextdata send to callback
    (MenuLayerCallbacks){
      .get_num_sections = 0, //0 defaults to 1 section
      .get_num_rows = getNumberOfStations,
      .get_cell_height = getCellHeight, // 0 default of 44, ist zu viel
      .get_header_height = 0, //0 switches header of
      .draw_header = 0, //can be 0 if get_header_height is set to 0
      .draw_row = drawStationRaw,
      .select_click = stationSelect, //for simple menu only 3 callbacks are neccessary
      .selection_changed = 0,// stationSelectChanged,
    });
    layer_add_child(window_get_root_layer(menu_window),(Layer *) ml_Stations);
    menu_layer_set_click_config_onto_window(ml_Stations, menu_window);
  setLayersVisibility();
  //evtl. Informationen vom Device abholen, nein hier zu früh
}


//wird von s_window unload gerufen
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
  menu_layer_destroy(ml_Stations);
  window_destroy(s_window);
  window_destroy(menu_window);
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

void resetGui()
{
	mode=none;
	rearrangeGui();
}
void rearrangeGui()
{
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "call to rearrangeGui");
	//Null is the same as clear
	resetText();
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
		if (device == RaspiRadio)
		{
			text_layer_set_text(tl_Station,raspiFullStatus[0]);
			text_layer_set_text(tl_Info,raspiFullStatus[1]);
		}
		break;
	}
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_UP, s_res_pngStation);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_SELECT, s_res_pngPower);
	action_bar_layer_set_icon(s_actionbarlayer_1, BUTTON_ID_DOWN, s_res_pngVolume);
	if (device == RaspiRadio && ! timerAktualisiereStatus)
		aktualisiereStatus();
	setLayersVisibility();
}
//hauptfenster
static void handle_window_unload(Window* window) {
  destroy_ui();
  //APP_LOG(APP_LOG_LEVEL_INFO,"Done is %i warnown is %i minutes is %i ",done ? 1 : 0, 
  //                       warnown ? 1 : 0, minutes);
  //APP_LOG(APP_LOG_LEVEL_INFO,"Destroy window");
}
//menu_fenster, wenn der Nutzer den back button im menu drueckt
//fenster wird nicht zerstört, denn ich kann es später noch nutzen?
static void handle_menu_window_unload(Window * w)
{
	 //APP_LOG(APP_LOG_LEVEL_INFO,"unload of menu_window");
	mode = none; //umschalten auf haupwindow
	getFullStatus();
	rearrangeGui();
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

//handler ------ im modus station fuer device raspberry greifen die menu-handler
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
			break;
		case program:
			mode=none;
			switchOffAccel();
			sendExit();
			rearrangeGui();
			break;
	}
}

//select long click - previous
//switch controlled device - we have only to in the moment, so we
//switch between them
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
	 //APP_LOG(APP_LOG_LEVEL_INFO,"long select click with raspio=%d and vu=%d",raspio,vu);
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
			{
				getVolume();
			}
			rearrangeGui();
			break;
	  case volume:
			sendVolDown();
			break;
	  case program:
			sendProgramDown();
			break;
	}
}
//up click
static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //... called on single click ...
  //Window *window = (Window *)context;	
  switch (mode)
	{
  	  case none:
			mode = program;
			if (device == Vu)
			{
				sendProgramDown();
				sendProgramUp(); //back to original position
			}
			else if (device == RaspiRadio)
			{
				getStationList();
			}
			rearrangeGui();
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

}
//misc
static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //... called on single click ...
  //Window *window = (Window *)context;
	//APP_LOG(APP_LOG_LEVEL_INFO,"select click with raspio=%d and vu=%d",raspio,vu);;
	switch ( mode)
	{
		case volume:
			mode = none; //just go back
			switchOffAccel();
			break;
		case none:
			switchOffAccel();
			if (device == Vu)
				sendPower();
			else if (device == RaspiRadio)
				raspiSwitchPlay();
			break;
		case program:
			switchOffAccel();
			sendOK();
			break;
	}
	rearrangeGui();
}
//------------------------------------------------------------
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

//war ein provider fuer das window, nutze jetzt actionbar
/*
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
*/
void show_control(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_set_window_handlers(menu_window,(WindowHandlers)
		  {
		  	  .unload = handle_menu_window_unload
		  });
  window_stack_push(s_window, true);
  //APP_LOG(APP_LOG_LEVEL_INFO,"pushed on stack");

}

void hide_control(void) {
  window_stack_remove(s_window, true);
  freeArray(raspiFullStatus,RFSZahl);
  freeArray(stationList,numberOfStations);
}
/*
void bind_clicks(void)
{
	  window_set_click_config_provider(s_window, (ClickConfigProvider) config_provider);
}
*/
