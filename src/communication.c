#include <pebble.h>
#include "communication.h"
#include "globals.h"
#include "control.h"

int numberOfStations = 0;
char **stationList = 0;

//free arrays, intended to use on customHours and customRels
void freeArray(char **arr,int n)
{
  if (arr)
  {
	  for (int i = 0; i< n; ++i)
	  {
		  free(arr[i]);
		  arr[i]=0;
	  }
  }
}


//put string in the form bla | blub and blob | xyz to array
//n is number of words in the array arrOfC, n char * must be
//present in the array
//have array of given size of type char *
char ** string_to_array(char ** arrOfC, int nOld, int nNew, char *string)
{
   freeArray(arrOfC,nOld);
   if (nOld != nNew || !arrOfC )
   {
	   arrOfC = realloc(arrOfC,nNew * sizeof(char *));
   }
   if (!arrOfC)
   {
   	  APP_LOG(APP_LOG_LEVEL_INFO, "no room left") ;
   }
   else
   {
	   char * akt = strchr(string, '|');
	   char * prev = string;
	   int i = 0;
	   while (akt && i < nNew-1)
	   {
		   int size = akt - prev;
		   arrOfC[i] = (char * ) calloc(size+1, sizeof(char));
		   if (!arrOfC[i])
			   	  APP_LOG(APP_LOG_LEVEL_INFO, "no room left") ;
		   else
		   {
			   	  strncpy(arrOfC[i],prev, size);
			   	   //APP_LOG(APP_LOG_LEVEL_INFO, "pointer is %p and String is %s size is %d", arrOfC[i],arrOfC[i],size) ;
			   	  i++;
		   }
		   prev = akt + 1;
		   akt = strchr(akt+1,'|');
	   }
	   //letzten holen
	   int size = string + strlen(string) - prev;
	   arrOfC[i] = (char * ) calloc(size+1, sizeof(char));
	   strcpy(arrOfC[i++],prev);
	   }
   return arrOfC;
   //log_array(arrOfC,i);
}

//aus der Liste der empfangenen Stationen ein array aus strings bauen
void generateStationMenu( char * string)
{
	//zaehlen
	int oldNumber = numberOfStations;
	numberOfStations = 0;
	for ( unsigned int i = 0 ; i < strlen(string)  ; ++i)
    	if ( string[i]=='|')
    		numberOfStations++;
    numberOfStations++;

    //array mit den Strings
    stationList = string_to_array(stationList,oldNumber,numberOfStations,string);
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  char * liste =0 ;
  bool newConfig = false;

  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
   //APP_LOG(APP_LOG_LEVEL_INFO, "received message ") ;
  while (t != NULL) {
	switch (t->key)
    {
	case KEY_APPREADY:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message appready have device %d", (int) device) ;
    	//wenn im raspberry mode frage den status ab
    	if (device == RaspiRadio)
    		getFullStatus();
    	else if (device == NotSet)
    	{
    		APP_LOG(APP_LOG_LEVEL_INFO, "try to get the options");
    		getOptions();
    	}
    	break;
    case KEY_VU:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message with vu %d ", (int) t->value->uint8) ;
    	vu = t->value->uint8 == 1;
    	persist_write_bool(KEY_VU,vu);
    	break;
    case KEY_VUIP :
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message with vu ip %s ", (char *) t->value->cstring) ;
    	newConfig = true;
    	break;
    case KEY_RASPIO:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message with raspio %d ", (int) t->value->uint8) ;
    	raspio = t->value->uint8 == 1;
    	persist_write_bool(KEY_RASPIO,raspio);
    	break;
    case KEY_RASPIP:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message with rasp ip %s", (char *) t->value->cstring) ;
    	break;
    case KEY_ACCEL:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received message with accel %d ", (int) t->value->uint8) ;
    	accel = t->value->uint8 == 1;
    	persist_write_bool(KEY_ACCEL,accel);
    	break;
    case KEY_RASPVOLUME:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received volume from pi  %d ", (int) t->value->uint8) ;
    	snprintf(textVolRaspi,4,"%d",t->value->uint8); //snprintf statt sprintf
    	rearrangeGui();
    	break;
    case KEY_RASPFULLSTATUS:
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received fullstatus from pi  %s ", (char *) t->value->cstring) ;
    	raspiFullStatus = string_to_array(raspiFullStatus,RFSZahl,RFSZahl,t->value->cstring);
    	rearrangeGui();
    	break;
    case KEY_RASPSTATIONLIST:
    	liste = t->value->cstring;
    	 //APP_LOG(APP_LOG_LEVEL_INFO, "received stationlist from pi  %s ", liste) ;
    	generateStationMenu(liste);
    	redrawStationMenu();
    	break;
    case  KEY_RASPACTUALSTAION:
    	actualStation = (int) t->value->uint8;
    	break;
    }
    t = dict_read_next(iterator);
  }
  if (newConfig) //schalte auf eingangsbildschirm
  {
	  if (vu && !raspio )
		  device = KEY_VU;
	  else if (raspio)
		  device = KEY_RASPIO;
	  resetGui();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


//send key to phone
static void sendSignal(int key)
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter, can't send");
  }
  else
  {
    Tuplet tuple =  TupletInteger(KEY_CONTROL, key);
    dict_write_tuplet(iter, &tuple);
    dict_write_end(iter);
    app_message_outbox_send(); //ok laeuft pebble-js-app.js reagiert
  }
}
static void sendKeyVal(int key , int val )
{
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter, can't send");
	}
	else
	{
		Tuplet tuple =  TupletInteger(key, val);
		dict_write_tuplet(iter, &tuple);
		dict_write_end(iter);
		   //APP_LOG(APP_LOG_LEVEL_INFO, "send key %d and val %d", key,val);
		app_message_outbox_send(); //ok laeuft pebble-js-app.js reagiert
	}
}


void sendPower(){sendSignal(KEY_POWER);}
void sendVolUp(){sendSignal(KEY_VOLUP);}
void sendVolDown(){sendSignal(KEY_VOLDOWN);}
void sendProgramUp(){sendSignal(KEY_STATIONUP);}
void sendProgramDown(){sendSignal(KEY_STATIONDOWN);}
void sendOK(){sendSignal(KEY_OK);}
void sendExit(){sendSignal(KEY_EXIT);}


//communication with the Raspi
void getVolume(){sendSignal(KEY_RASPVOLUME);}
void getFullStatus(){sendSignal(KEY_RASPFULLSTATUS);}
void getStationList(){sendSignal(KEY_RASPSTATIONLIST);}
void getOptions(){sendKeyVal(KEY_OPTIONS,1);}

//send new device to phone
void sendNewDevice()
{
	sendKeyVal(KEY_DEVICE,device);
}
//switch station on raspi
void switchStation()
{
	sendKeyVal(KEY_SWITCHSTATION,actualStation);
}


void initCommunication(void)
{
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());	
}
