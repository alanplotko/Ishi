#include <pebble.h>
#include <stringtok.h>
	
#define KEY_BUTTON    0
	
#define KEY_VIBRATE   1
	
#define KEY_ACTION     2
#define ACTION_ANS     0
#define ACTION_EASE    1
#define ACTION_Q       2
#define ACTION_DECK_SELECT    3

#define KEY_DECKS 3
#define KEY_QUESTION 4
#define KEY_ANSWER 5
#define KEY_EASE 6
	
#define NUM_MENU_SECTIONS  1
#define DECKS  5
#define DECK_MENU_SIZE  10
  
static Window *s_main_window;
static TextLayer *s_text_layer;
static unsigned int stage = ACTION_DECK_SELECT;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[DECK_MENU_SIZE];
static int num_menu_items = 0;
static GBitmap *s_menu_icon_image;

static void menu_select_callback(int index, void *ctx) {
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

/******************************* Build menu **********************************/

static void build_menu(Window *window, char *menu_str, int len) {  
  char *start = menu_str;
  char *end = menu_str;
  int substr_len = 0;
  int total = 0;
  
  s_menu_icon_image = gbitmap_create_with_resource(RESOURCE_ID_INDEX_CARD);
	
  while (*start != '\0' && total < len - 1 && num_menu_items < DECK_MENU_SIZE) {
	while (*end != ';')
	{
		end++;
		substr_len++;
	}
	char *buf = malloc(substr_len + 1);
	memcpy(buf, start, substr_len);
	buf[substr_len] = '\0';
    s_first_menu_items[num_menu_items++] = (SimpleMenuItem) {
      .title = buf,
      .callback = menu_select_callback,
      .icon = s_menu_icon_image,
    };
	//free(buf);
	end++;
  	start = end;
	total += substr_len + 1;
	substr_len = 0;
  }
  /*s_first_menu_items[num_menu_items++] = (SimpleMenuItem) {
      .title = "CS",
      .callback = menu_select_callback,
      .icon = s_menu_icon_image,
  };
  s_first_menu_items[num_menu_items++] = (SimpleMenuItem) {
      .title = "Math",
      .callback = menu_select_callback,
      .icon = s_menu_icon_image,
  };
  s_first_menu_items[num_menu_items++] = (SimpleMenuItem) {
      .title = "Physics",
      .callback = menu_select_callback,
      .icon = s_menu_icon_image,
  };*/
  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = num_menu_items,
    .items = s_first_menu_items,
  };
  Layer *window_layer = window_get_root_layer(window);
  layer_remove_child_layers(window_layer);
  GRect bounds = layer_get_frame(window_layer);
  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

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
      case KEY_DECKS:
        // Build menu
        text_layer_set_text(s_text_layer, t->value->cstring);
        build_menu(s_main_window, t->value->cstring, t->length);
        break;
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
    case ACTION_DECK_SELECT:
      break;
    case ACTION_Q:
      text_layer_set_text(s_text_layer, "Stage: Answer");
      stage = ACTION_ANS;
      send(KEY_ACTION, ACTION_ANS);
      break;
    case ACTION_ANS:
      text_layer_set_text(s_text_layer, "Stage: Results");
      stage = ACTION_EASE;
      send(KEY_ACTION, ACTION_EASE);
      break;
    case ACTION_EASE:
      text_layer_set_text(s_text_layer, "Stage: Question");
      stage = ACTION_Q;
      send(KEY_ACTION, ACTION_Q);
      break;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(stage != ACTION_DECK_SELECT) {
    text_layer_set_text(s_text_layer, "Clicked up");
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if(stage != ACTION_DECK_SELECT) {
    text_layer_set_text(s_text_layer, "Clicked down");
  }
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
  Layer *window_layer = window_get_root_layer(window);
  s_text_layer = text_layer_create(GRect(0, 57, 144, 168));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_text_layer, "Loading decks...");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void main_window_unload(Window *window) {
  // Destroy main TextLayer
  text_layer_destroy(s_text_layer);
  simple_menu_layer_destroy(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image);
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
  
  // Ask Android app for decks
  send(KEY_ACTION, ACTION_DECK_SELECT);
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
