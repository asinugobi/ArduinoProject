#include <pebble.h>

static Window *window;
static TextLayer *hello_layer;
static char msg[100];
static AppTimer *s_progress_timer;
static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_sections[2];
static SimpleMenuItem s_color_menu_items[3];
static SimpleMenuItem s_temp_menu_items[3];
static Window *menu_window;

enum {
  MESSAGE = 0
};

void out_sent_handler(DictionaryIterator *sent, void *context) { 
  // outgoing message was delivered -- do nothing
}
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  text_layer_set_text(hello_layer, "Error out!");
}

void in_received_handler(DictionaryIterator *received, void *context) {
  // incoming message received
  // looks for key #0 in the incoming message 
  int key = 0;
  Tuple *text_tuple = dict_find(received, key); 
  if (text_tuple) {
    if (text_tuple->value) {
      // put it in this global variable 
      strcpy(msg, text_tuple->value->cstring);
    } 
    else strcpy(msg, "no value!");
    text_layer_set_text(hello_layer, msg); 
  } else {
    text_layer_set_text(hello_layer, "no message!");
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  text_layer_set_text(hello_layer, "Error in!");
}

void make_request(char* request_type){
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring (iter, MESSAGE, request_type);
  app_message_outbox_send();
}

void request_temperature_update(){
  make_request("getTemp");
  s_progress_timer = app_timer_register(1000, request_temperature_update, NULL);
}

void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_push(menu_window, true); 
}

/* This is called when the select button is clicked */
void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // text_layer_set_text(hello_layer, "Selected!"); 
  // request_temperature_update();
  make_request("toggleStandby");
}

/* This is called when the bottom button is clicked */
void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(hello_layer, "Requesting...");
  make_request("changeUnits");
}


/* Menu Functions */
static void color_menu_select_callback(int index, void *ctx) {
  // s_first_menu_items[index].subtitle = "You've hit select here!";
  // layer_mark_dirty(simple_menu_layer_get_layer(s_menu_layer));
  if(index == 0){
    make_request("blue");
  } else if(index == 1){
    make_request("green");
  } else if(index == 2){
    make_request("red");
  }
  window_stack_pop(true);
}

static void temp_menu_select_callback(int index, void *ctx) {
  // s_first_menu_items[index].subtitle = "You've hit select here!";
  // layer_mark_dirty(simple_menu_layer_get_layer(s_menu_layer));
  if(index == 0){
    make_request("avg");
  } else if(index == 1){
    make_request("max");
  } else if(index == 2){
    make_request("min");
  }
  window_stack_pop(true);
}

/* this registers the appropriate function to the appropriate button */
void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  hello_layer = text_layer_create((GRect) { 
    .origin = { 0, 72 },
    .size = { bounds.size.w, 20 } 
  }); 
  text_layer_set_text(hello_layer, "Welcome to Temp Reader"); 
  text_layer_set_text_alignment(hello_layer, GTextAlignmentCenter); 
  layer_add_child(window_layer, text_layer_get_layer(hello_layer));
  
  
  
  // Create the auto update timer
  s_progress_timer = app_timer_register(1000, request_temperature_update, NULL);
}

static void window_unload(Window *window) {
  text_layer_destroy(hello_layer);
}

static void menu_window_load(Window* window){
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the menu items
  s_color_menu_items[0] = (SimpleMenuItem) {
    .title = "Blue",
    .callback = color_menu_select_callback,
  };
  s_color_menu_items[1] = (SimpleMenuItem) {
    .title = "Green",
    .callback = color_menu_select_callback,
  };
  s_color_menu_items[2] = (SimpleMenuItem) {
    .title = "Red",
    .callback = color_menu_select_callback,
  };
  
  // Create the menu section
  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = 3,
    .items = s_color_menu_items,
  };
  
  // Create the menu items
  s_temp_menu_items[0] = (SimpleMenuItem) {
    .title = "Average",
    .callback = temp_menu_select_callback,
  };
  s_temp_menu_items[1] = (SimpleMenuItem) {
    .title = "High",
    .callback = temp_menu_select_callback,
  };
  s_temp_menu_items[2] = (SimpleMenuItem) {
    .title = "Low",
    .callback = temp_menu_select_callback,
  };
  
  // Create the menu section
  s_menu_sections[1] = (SimpleMenuSection) {
    .num_items = 3,
    .items = s_temp_menu_items,
  };
  
  // Create the menu
  s_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, 2, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window* window){
  simple_menu_layer_destroy(s_menu_layer);
}

static void init(void) {
  window = window_create(); 
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload, 
  });
  
  // need this for adding the listener
  window_set_click_config_provider(window, config_provider);
  
  
  // Menu window
  menu_window = window_create();
  window_set_window_handlers(menu_window, (WindowHandlers) {
    .appear = menu_window_load,
    .disappear = menu_window_unload
  });
  
  // for registering AppMessage handlers 
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64; 
  app_message_open(inbound_size, outbound_size);
  
  const bool animated = true;
  window_stack_push(window, animated); 
}

static void deinit(void) { 
  window_destroy(window);
  window_destroy(menu_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  }