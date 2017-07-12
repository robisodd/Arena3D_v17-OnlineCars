#include "global.h"

// ------------------------------------------------------------------------ //
//  PebbleKit JS Functions
// ------------------------------------------------------------------------ //
bool javascript_ready = false;

enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  USER_KEY = 2,
  SYSTEM_MESSAGE = 3,
  ERR_MESSAGE = 4,
  OBJECT_INDEX_KEY = 5,
  X_POSITION_KEY = 6,
  Y_POSITION_KEY = 7,
  FACING_KEY = 8,
  UPDATE_OBJECT_KEY = 9,
  DELETE_OBJECT_KEY = 12
};

enum {
  COMMAND_OPEN_CONNECTION = 0,
  COMMAND_CLOSE_CONNECTION = 1
};

// Write message to buffer & send
void send_status(int val) {
  if(!javascript_ready) return;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_int8(iter, STATUS_KEY, val);
	dict_write_end(iter);
  app_message_outbox_send();
}

void send_message(char *msg){
  if(!javascript_ready) return;
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	dict_write_cstring(iter, MESSAGE_KEY, msg);
	//dict_write_int8(iter, MESSAGE_KEY, 1);
  
	dict_write_end(iter);
  app_message_outbox_send();
}

void send_position() {
  if(!javascript_ready) return;
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	dict_write_int16(iter, X_POSITION_KEY, player.x);
  dict_write_int16(iter, Y_POSITION_KEY, player.y);
  dict_write_int16(iter, FACING_KEY, player.facing);
	dict_write_end(iter);
  app_message_outbox_send();
}

/*
https://developer.pebble.com/guides/communication/sending-and-receiving-data/#reading-an-incoming-message
var dict = {
  'Data': [1, 2, 4, 8, 16, 32, 64]
};

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  const int length = 32;  // Expected length of the binary data

  // Does this message contain the data tuple?
  Tuple *data_tuple = dict_find(iter, MESSAGE_KEY_Data);
  if(data_tuple) {
    uint8_t *data = data_tuple->value->data;  // Read the binary data value
    uint8_t byte_zero = data[0];              // Inspect the first byte, for example
    memcpy(s_buffer, data, length);           // Store into an app-defined buffer
  }
*/


// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  javascript_ready = true;
  static int current_index = 0;
	Tuple *tuple;
  //printf("C received message from JS");
  
  if((tuple = dict_find(received, UPDATE_OBJECT_KEY))) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv U");
    int16_t *data16 = (int16_t*)tuple->value->data;  // Read the binary data value
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "[index: %d, x: %d, y: %d, facing:%d]", data16[0], data16[1], data16[2], data16[3]);
    current_index = data16[0];
    object[current_index].type   = 2;
    object[current_index].sprite = 0;
    object[current_index].x      = data16[1];
    object[current_index].y      = data16[2];
    object[current_index].facing = data16[3];
  }

  if((tuple = dict_find(received, OBJECT_INDEX_KEY))) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv I: %d", (int)tuple->value->int16);
    current_index = tuple->value->int16;
    object[current_index].type = 1;
    object[current_index].sprite = 0;
  }

  if((tuple = dict_find(received, X_POSITION_KEY))) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv X: %d", (int)tuple->value->int16);
    //player.x = tuple->value->int16;
    object[current_index].x = tuple->value->int16;
  }

  if((tuple = dict_find(received, Y_POSITION_KEY))) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv Y: %d", (int)tuple->value->int16);
    //player.y = tuple->value->int16;
    object[current_index].y = tuple->value->int16;
  }

  if((tuple = dict_find(received, FACING_KEY))) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv F: %d", (int)tuple->value->int16);
    object[current_index].facing = tuple->value->int16;
    //player.facing = tuple->value->int16;
  }

  if((tuple = dict_find(received, DELETE_OBJECT_KEY))) {
    // Delete object
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Recv I: %d", (int)tuple->value->int16);
    object[tuple->value->int16].type = 0;
  }

  tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "Position: %d", (int)tuple->value->uint32);
    
    //snprintf(text, sizeof(text), "Position: %d", (int)tuple->value->uint32);
    //console_layer_write_text(console_layer, text, true);
    
    printf("Received Status: %d", (int)tuple->value->uint32);
    vibes_cancel(); vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t []){30}, .num_segments = 1});  // pulse
	}
	
	tuple = dict_find(received, USER_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "User: %s", tuple->value->cstring);
//     snprintf(text, sizeof(text), "%s:", tuple->value->cstring);
    //console_layer_set_font(console_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    //console_layer_set_text_color(console_layer, GColorBlue);
    //console_layer_write_text(console_layer, text, true);
//     console_layer_write_text_attributes(console_layer, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorVeryLightBlue, GColorClear, GTextAlignmentCenter, false, true);
    //console_layer_write_text_attributes(console_layer, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorBabyBlueEyes, GColorClear, GTextAlignmentCenter, false, true);
	}
  
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Message: %s", tuple->value->cstring);
//     snprintf(text, sizeof(text), "%s", tuple->value->cstring);
//     console_layer_write_text(console_layer, text, true);
	}
  
	tuple = dict_find(received, ERR_MESSAGE);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Error Message from JS: %s", tuple->value->cstring);
//     snprintf(text, sizeof(text), "%s", tuple->value->cstring);
//     console_layer_write_text_attributes(console_layer, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorRed, GColorClear, GTextAlignmentCenter, true, true);
	}
  
	tuple = dict_find(received, SYSTEM_MESSAGE);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "System Message: %s", tuple->value->cstring);
    //printf("Opening WebSocket");
    //send_status(COMMAND_OPEN_CONNECTION);

//     snprintf(text, sizeof(text), "%s", tuple->value->cstring);
//     console_layer_write_text_attributes(console_layer, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GColorGreen, GColorClear, GTextAlignmentCenter, false, true);
	}  
}

static char *translate_appmessageresult(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
//   error_msg(translate_appmessageresult(reason));
  printf("Error: %s", translate_appmessageresult(reason));
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
//   error_msg(translate_appmessageresult(reason));
  printf("Error: %s", translate_appmessageresult(reason));
}

void init_app_message() {
  // Init AppMessage
      // Register AppMessage handlers
      app_message_register_inbox_received(in_received_handler); 
      app_message_register_inbox_dropped(in_dropped_handler); 
      app_message_register_outbox_failed(out_failed_handler);
      // Initialize AppMessage inbox and outbox buffers with a suitable size
      const int inbox_size = 256;
      const int outbox_size = 256;
      app_message_open(inbox_size, outbox_size);
  // END Init AppMessage
}