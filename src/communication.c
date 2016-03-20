#include <pebble.h>
#include "communication.h"
#include "globals.h"



extern uint8_t device;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  // Process all pairs present
  APP_LOG(APP_LOG_LEVEL_INFO, "received message ") ;
  while (t != NULL) {
	switch (t->key)
    {
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
  //special key new device
  if (key == KEY_DEVICE)
  {
	  switch (device)
	  {
	  case Vu:
		  key = KEY_VU;
		  break;
	  case RaspiRadio:
		  key = KEY_RASPIO;
		  break;
	  }
  }
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
void sendArrowUp(){sendSignal(KEY_ARRUP);}
void sendArrowDown(){sendSignal(KEY_ARRDOWN);}
void sendOK(){sendSignal(KEY_OK);}
void sendExit(){sendSignal(KEY_EXIT);}
void sendNewDevice(){sendSignal(KEY_DEVICE);}
 
void initCommunication(void)
{
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());	
}
