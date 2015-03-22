#include <pebble.h>
	
#define KEY_BUTTON    0
#define KEY_VIBRATE   1
#define KEY_ACTION    2

#define ACTION_ANS    0
#define ACTION_EASE   1
#define ACTION_Q      2
#define ACTION_DECK_SELECT  3

#define KEY_DECKS  3
#define KEY_QUESTION  4
#define KEY_ANSWER  5
#define KEY_EASE  6
	
#define NUM_MENU_SECTIONS  1
#define DECKS  5
#define DECK_MENU_SIZE  10
  
static Window *s_main_window;
static Window *s_question_window;

static MenuLayer *s_menu_layer;
static TextLayer *s_text_layer;
static char *s_menu_titles[DECK_MENU_SIZE];
static int num_menu_items = 0;
static GBitmap *s_menu_icon_image;

/******************************* Build menu **********************************/

static void load_menu_titles(char *menu_str) {
	char *start = menu_str;
  	char *end = menu_str;
	int substr_len = 0;
	char *buf;
	while (*start != '\0' && num_menu_items < DECK_MENU_SIZE) {
		while (*end != ';') {
			end++;
			substr_len++;
	    }
	    
		buf = malloc(substr_len + 1);
	    memcpy(buf, start, substr_len);
	    buf[substr_len] = '\0';
		s_menu_titles[num_menu_items] = buf;
		
		end++;
		start = end;
		substr_len = 0;
		num_menu_items++;
	}
}

static void destroy_menu_titles() {
	for (int i = 0; i < num_menu_items; i++) {
		free(s_menu_titles[i]);
	}
}

/******************************* AppMessage ***********************************/

static void send(int key, int message) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &message, sizeof(int), true);
  app_message_outbox_send();
}

static void sendDeckName(int key, const char *message) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, key, message);
  app_message_outbox_send();
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  // Get window from context
  Window *window = (Window *)context;
  Layer *window_layer = window_get_root_layer(window);
  
  // Get the first pair
  Tuple *t = dict_read_first(iterator);
  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch(t->key) {
      // Build menu
      case KEY_DECKS:
		    load_menu_titles(t->value->cstring);
		    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
        break;
      case KEY_QUESTION:
        text_layer_set_text(s_text_layer, t->value->cstring);
        layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
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

/******************************* question_window **********************************/

static void question_window_load(Window *window) {
  // Create text layer with question text
  Layer *window_layer = window_get_root_layer(window);
  send(KEY_ACTION, ACTION_Q);
}

static void question_window_unload(Window *window) {
  
}

/********************************* Buttons ************************************/

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  sendDeckName(KEY_DECKS, s_menu_titles[cell_index->row]);
  s_question_window = window_create();
  window_set_window_handlers(s_question_window, (WindowHandlers) {
    .load = question_window_load,
    .unload = question_window_unload,
  });
  window_stack_push(s_question_window, true);
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
   return num_menu_items;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, s_menu_titles[cell_index->row], NULL, s_menu_icon_image);
  menu_layer_reload_data(s_menu_layer);
}

/******************************* main_window **********************************/

static void main_window_load(Window *window) {
  // Create text layer with "Loading decks..." message
  Layer *window_layer = window_get_root_layer(window);
  s_text_layer = text_layer_create(GRect(0, 57, 144, 168));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_text_layer, "Loading decks...");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  
  // Index card icon
  s_menu_icon_image = gbitmap_create_with_resource(RESOURCE_ID_INDEX_CARD);
  
  // Now we prepare to initialize the menu layer
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    //*.get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    //*.get_header_height = menu_get_header_height_callback,
    //*.draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  
  // Remove "Loading decks..." message
  layer_remove_child_layers(window_layer);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
	
  // Ask Android app for decks
  send(KEY_ACTION, ACTION_DECK_SELECT);
  //sendDeckName(KEY_DECKS, "PEBBLE");
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  gbitmap_destroy(s_menu_icon_image);
  menu_layer_destroy(s_menu_layer);
  destroy_menu_titles();
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
