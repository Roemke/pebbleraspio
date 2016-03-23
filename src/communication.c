#include <pebble.h>
#include "communication.h"
#include "globals.h"
#include "control.h"


//free arrays, intended to use on customHours and customRels
void freeArray(char **arr,int n)
{
  for (int i = 0; i< n; ++i)
  {
	  free(arr[i]);
	  arr[i]=0;
  }
}

//put string in the form bla | blub and blob | xyz to array
//n is number of words in the array arrOfC, n char * must be
//present in the array
//have array of given size of type char *
static void string_to_array(char ** arrOfC, int n, char *string)
{
   freeArray(arrOfC,n);
   char * akt = strchr(string, '|');
   char * prev = string;
   int i = 0;
   while (akt && i < n-1)
   {
       int size = akt - prev;
       if (arrOfC[i])
         free(arrOfC[i]);
       arrOfC[i] = (char * ) calloc(size+1, sizeof(char));
       strncpy(arrOfC[i++],prev, size);
       prev = akt + 1;
       akt = strchr(akt+1,'|');
   }
   //letzten holen
   int size = string + strlen(string) - prev;
   if (arrOfC[i])
     free(arrOfC[i]);
   arrOfC[i] = (char * ) calloc(size+1, sizeof(char));
   strcpy(arrOfC[i++],prev);
   //log_array(arrOfC,i);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  // Process all pairs present
  APP_LOG(APP_LOG_LEVEL_INFO, "received message ") ;
  while (t != NULL) {
	switch (t->key)
    {
	case KEY_APPREADY:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message appready ") ;
    	//wenn im raspberry mode frage den status ab
    	if (device == RaspiRadio)
    		getFullStatus();
		break;
    case KEY_VU:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message with vu %d ", (int) t->value->uint8) ;
    	vu = t->value->uint8 == 1;
    	persist_write_bool(KEY_VU,vu);
    	break;
    case KEY_VUIP :
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message with vu ip %s ", (char *) t->value->cstring) ;
    	break;
    case KEY_RASPIO:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message with raspio %d ", (int) t->value->uint8) ;
    	raspio = t->value->uint8 == 1;
    	persist_write_bool(KEY_RASPIO,raspio);
    	break;
    case KEY_RASPIP:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message with rasp ip %s", (char *) t->value->cstring) ;
    	break;
    case KEY_ACCEL:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received message with accel %d ", (int) t->value->uint8) ;
    	accel = t->value->uint8 == 1;
    	persist_write_bool(KEY_ACCEL,accel);
    	break;
    case KEY_RASPVOLUME:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received volume from pi  %d ", (int) t->value->uint8) ;
    	snprintf(textVolRaspi,4,"%d",t->value->uint8); //snprintf statt sprintf
    	rearrangeGui();
    	break;
    case KEY_RASPFULLSTATUS:
    	APP_LOG(APP_LOG_LEVEL_INFO, "received fullstatus from pi  %s ", (char *) t->value->cstring) ;
    	string_to_array(raspiFullStatus,RFSZahl,t->value->cstring);
    	rearrangeGui();
    	break;
    }
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
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
void sendPower(){sendSignal(KEY_POWER);}
void sendVolUp(){sendSignal(KEY_VOLUP);}
void sendVolDown(){sendSignal(KEY_VOLDOWN);}
void sendProgramUp(){sendSignal(KEY_STATIONUP);}
void sendProgramDown(){sendSignal(KEY_STATIONDOWN);}
void sendOK(){sendSignal(KEY_OK);}
void sendExit(){sendSignal(KEY_EXIT);}

//communication with the Raspi
void getVolume(){sendSignal(KEY_RASPVOLUME);		APP_LOG(APP_LOG_LEVEL_DEBUG, "send request for volumee");
}
void getFullStatus(){sendSignal(KEY_RASPFULLSTATUS);}

//send new device to phone
void sendNewDevice()
{
  //special key new device
	int key=RaspiRadio;
	switch (device)
	{
	case Vu:
	  key = KEY_VU;
	  break;
	case RaspiRadio:
	  key = KEY_RASPIO;
	  break;
	}
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter, can't send");
	}
	else
	{
		Tuplet tuple =  TupletInteger(KEY_DEVICE, key);
		dict_write_tuplet(iter, &tuple);
		dict_write_end(iter);
		app_message_outbox_send(); //ok laeuft pebble-js-app.js reagiert
	}
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
