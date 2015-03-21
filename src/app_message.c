#include <pebble.h>

#define KEY_BUTTON    0
#define KEY_VIBRATE   1
#define KEY_ACTION    2
  
#define STAGE_ANS     0
#define STAGE_RES     1
#define STAGE_QTN     2

static Window *s_main_window;
static TextLayer *s_text_layer;
static unsigned int stage = STAGE_QTN;

/******************************* AppMessage ***********************************/

static void send(int key, int message) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &message, sizeof(int), true);
  app_message_outbox_send();
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch(t->key) {
      case KEY_VIBRATE:
        // Trigger vibration
        text_layer_set_text(s_text_layer, "Vibrate!");
        vibes_short_pulse();
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_INFO, "Unknown key: %d", (int)t->key);
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/********************************* Buttons ************************************/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(stage) {
    case STAGE_QTN:
      text_layer_set_text(s_text_layer, "Stage: Answer");
      stage = STAGE_ANS;
      send(KEY_ACTION, STAGE_ANS);
      break;
    case STAGE_ANS:
      text_layer_set_text(s_text_layer, "Stage: Results");
      stage = STAGE_RES;
      send(KEY_ACTION, STAGE_RES);
      break;
    case STAGE_RES:
      text_layer_set_text(s_text_layer, "Stage: Question");
      stage = STAGE_QTN;
      send(KEY_ACTION, STAGE_QTN);
      break;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Clicked up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Clicked down");
}

static void click_config_provider(void *context) {
  // Assign button handlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

/******************************* main_window **********************************/

static void main_window_load(Window *window) {
  // Create main TextLayer
  s_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_text_layer, "Stage: Question");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_text_layer));
}

static void main_window_unload(Window *window) {
  // Destroy main TextLayer
  text_layer_destroy(s_text_layer);
}

static void init(void) {
  // Register callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Create main Window
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
