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
static Window *s_ease_window;

static MenuLayer *s_menu_layer, *s_ease_menu_layer;
static TextLayer *s_text_layer, *s_question_text_layer;
static ScrollLayer *s_scroll_layer;
static char *s_menu_titles[DECK_MENU_SIZE];
static const char *s_ease_menu_titles[] = {"Again", "Hard", "Good", "Easy"};
static int s_ease;
static int num_menu_items = 0;
static GBitmap *s_menu_icon_image;

static int s_study_stage = 0;

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

/******************************* ease_window ***********************************/

static void ease_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
   if (cell_index->row == 0) {
	   send(KEY_EASE, 1);
   }
   switch(s_ease) {
    case 2:
      if(cell_index->row == 1) {
        send(KEY_EASE, 3);
      }
      break;
    case 3:
      if(cell_index->row == 1) {
        send(KEY_EASE, 3);
      }
      else if(cell_index->row == 2) {
        send(KEY_EASE, 4);
      }
      break;
    case 4:
      if(cell_index->row == 1) {
        send(KEY_EASE, 2);
      }
      else if(cell_index->row == 2) {
        send(KEY_EASE, 3);
      } 
      else if(cell_index->row == 3) {
        send(KEY_EASE, 4);
      } 
      break;
   }
   window_stack_pop(true);
}

static uint16_t ease_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
   return s_ease;
}

static void ease_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if(cell_index->row == 0) {
    menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[0], NULL, NULL);
  }
  switch(s_ease) {
    case 2:
      if(cell_index->row == 1) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[2], NULL, NULL);
      }
      break;
    case 3:
      if(cell_index->row == 1) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[2], NULL, NULL);
      }
      else if(cell_index->row == 2) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[3], NULL, NULL);
      }
      break;
    case 4:
      if(cell_index->row == 1) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[1], NULL, NULL);
      }
      else if(cell_index->row == 2) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[2], NULL, NULL);
      } 
      else if(cell_index->row == 3) {
        menu_cell_basic_draw(ctx, cell_layer, s_ease_menu_titles[3], NULL, NULL);
      } 
      break;
  }
  menu_layer_reload_data(s_ease_menu_layer);
}

static void s_ease_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  // Now we prepare to initialize the menu layer
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_ease_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_ease_menu_layer, NULL, (MenuLayerCallbacks){
    //*.get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = ease_menu_get_num_rows_callback,
    //*.get_header_height = menu_get_header_height_callback,
    //*.draw_header = menu_draw_header_callback,
    .draw_row = ease_menu_draw_row_callback,
    .select_click = ease_menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_ease_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_ease_menu_layer));
}

static void s_ease_window_unload(Window *window) {
  menu_layer_destroy(s_ease_menu_layer);
}

/******************************* AppMessage ***********************************/

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {  
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
      // Question or answer
      case KEY_QUESTION:
      case KEY_ANSWER:
        text_layer_set_text(s_question_text_layer, t->value->cstring);
		    layer_mark_dirty(text_layer_get_layer(s_question_text_layer));
        break;
      case KEY_EASE:
        s_ease = t->value->uint32;
        s_ease_window = window_create();
        window_set_window_handlers(s_ease_window, (WindowHandlers) {
          .load = s_ease_window_load,
          .unload = s_ease_window_unload,
        });
        window_stack_push(s_ease_window, true);
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
  send(KEY_ACTION, ACTION_Q);
  // Create text layer with question text
  Layer *window_layer = window_get_root_layer(window);
  s_question_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_font(s_question_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text(s_question_text_layer, "Loading Question...");
  text_layer_set_text_alignment(s_question_text_layer, GTextAlignmentCenter); 
  
  GRect bounds = layer_get_frame(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);
  s_scroll_layer = scroll_layer_create(bounds);
  
  GSize max_size = text_layer_get_content_size(s_question_text_layer);
  text_layer_set_size(s_question_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));

  // Add the layers for display
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_question_text_layer));
  
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void question_window_unload(Window *window) {
  text_layer_destroy(s_question_text_layer);
  scroll_layer_destroy(s_scroll_layer);
}

/********************************* Buttons ************************************/

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch(s_study_stage) {
    case 0:
      send(KEY_ACTION, ACTION_ANS);
      s_study_stage++;
      break;
    case 1:
      send(KEY_ACTION, ACTION_EASE);
      s_study_stage = 0;
      break;
  }
}

void config_provider(Window *window) {
  // Single click select button
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  s_question_window = window_create();
  s_study_stage = 0;
  // Click handlers
  window_set_click_config_provider(s_question_window, (ClickConfigProvider) config_provider);
  window_set_window_handlers(s_question_window, (WindowHandlers) {
    .load = question_window_load,
    .unload = question_window_unload,
  });
  window_stack_push(s_question_window, true);
  sendDeckName(KEY_DECKS, s_menu_titles[cell_index->row]);
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
