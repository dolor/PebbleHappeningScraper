#include <pebble.h>
#include <time.h>

#define KEY_CURRENT_COUNT 0
#define KEY_CURRENT_TIME 1
  
Window *window;
TextLayer *s_count_layer;
TextLayer *s_fetch_layer;
TextLayer *s_time_layer;
TextLayer *s_title_layer;
static char installs_layer_buffer[8];
static char installs_buffer[8];
static char time_buffer[16];
GFont *title_font;
GFont *count_font;

static void set_fetching(bool fetching, bool failed) {
    if (fetching)
        text_layer_set_text(s_fetch_layer, "fetching");
    else if (failed)
        text_layer_set_text(s_fetch_layer, "failed");
    else
        text_layer_set_text(s_fetch_layer, "");
}

static void update_count(int count, unsigned prevtime) {
    set_fetching(false, false);
    snprintf(installs_buffer, sizeof(installs_buffer), "%d", count);
    snprintf(installs_layer_buffer, sizeof(installs_layer_buffer), "%s", installs_buffer);
    text_layer_set_text(s_count_layer, installs_layer_buffer);
    
    unsigned curtime = time(NULL);
    unsigned timediff = curtime - prevtime;
    int hours = timediff / 3600;
    int minutes = (timediff - (hours * 3600)) / 60;
    snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d ago", hours, minutes);
    text_layer_set_text(s_time_layer, time_buffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  int predicted_time = -1;
  int predicted_count = -1;
  int current_count = -1;
  int current_time = -1;
  
  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_CURRENT_COUNT:
        current_count = (int)t->value->int32;
        break;
      case KEY_CURRENT_TIME:
        current_time = (int)t->value->int32;
        break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (current_count >= 0 && current_time >= 0) {
    update_count(current_count, current_time);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    set_fetching(false, true);
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static char *split_plugin_title(char *title) {
  int i;
  for (i=0; title[i]!= '\0'; i++) {
    if (title[i] == ' ')
      title[i] = '\n';
  }
  return title;
}

static Window *create_plugin_window(char *title) {
    Window *window = window_create();
    window_set_background_color(window, GColorWhite);
    GSize window_size = layer_get_bounds(window_get_root_layer(window)).size;

    // Static Title
    s_title_layer = text_layer_create(GRect(0, 0, 144, 196));
    title_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    text_layer_set_font(s_title_layer, title_font);
    text_layer_set_text(s_title_layer, split_plugin_title(title));
    text_layer_set_background_color(s_title_layer, GColorCobaltBlue);
    text_layer_set_text_color(s_title_layer, GColorWhite);
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);

    // Set to correct size
    GSize title_size = text_layer_get_content_size(s_title_layer);
    text_layer_set_size(s_title_layer, GSize(144, title_size.h + 10));

    // Prepare the counter text
    int remaining_height = window_size.h - title_size.h;
    s_count_layer = text_layer_create(GRect(0, 0, 144, window_size.h));
    text_layer_set_background_color(s_count_layer, GColorClear);
    text_layer_set_text_alignment(s_count_layer, GTextAlignmentCenter);
    text_layer_set_text_color(s_count_layer, GColorBlack);
    count_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
    text_layer_set_font(s_count_layer, count_font);
    text_layer_set_text(s_count_layer, "0");
    
    // Center
    int used_height = text_layer_get_content_size(s_count_layer).h;
    int count_y = window_size.h - remaining_height/2 - used_height/2;
    layer_set_frame(text_layer_get_layer(s_count_layer), GRect(0, count_y, 144, used_height));
    text_layer_set_size(s_count_layer, GSize(144, used_height));
  
    // Loading indicator
    s_fetch_layer = text_layer_create(GRect(0, window_size.h - 20, window_size.w, 20));
    text_layer_set_text_alignment(s_fetch_layer, GTextAlignmentLeft);
    text_layer_set_background_color(s_fetch_layer, GColorClear);
    text_layer_set_text_color(s_fetch_layer, GColorDarkGray);
    text_layer_set_text(s_fetch_layer, "fetched");
    
    // Display of last update time
    s_time_layer = text_layer_create(GRect(0, window_size.h - 16, window_size.w, 16));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorDarkGray);
    text_layer_set_text(s_time_layer, "1h ago");

    // Add to window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_title_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_count_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_fetch_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
    
    return window;
}

void handle_init(void) {
    window = create_plugin_window("Happening against Humanity");
    window_stack_push(window, true);

    set_fetching(true, false);
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void handle_deinit(void) {
  text_layer_destroy(s_count_layer);
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
