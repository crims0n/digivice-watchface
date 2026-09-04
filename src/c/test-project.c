#include <pebble.h>
/* TODO:
-add date
-animated sprite
-weather info?

target screen size 144*168
*/

/* GLOBAL VARIABLES */
static Window *s_window;

static TextLayer *s_time_layer;
static TextLayer *s_seconds_layer;
static TextLayer *s_meridiem_layer;
static TextLayer *s_date_layer;

static GFont s_large_font;
static GFont s_medium_font;
static GFont s_small_font;

static Layer *s_arrows_layer;
static GBitmap *s_arrow_dark_bitmap;
static GBitmap *s_arrow_filled_dark_bitmap;
static int s_current_seconds = 0;

const int MAX_SCREEN_X = 144;
const int MAX_SCREEN_Y = 168;

const int time_textbox_height = 60;
const int time_textbox_draw_y = 32;
const int date_textbox_height = 40;
const int date_textbox_draw_y = 128;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  s_current_seconds = tick_time->tm_sec;
  if (s_arrows_layer) {
    layer_mark_dirty(s_arrows_layer);
  }

  // Write the current hours and minutes into a buffer
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Write the current seconds into a buffer
  static char s_buffer_seconds[4];
  strftime(s_buffer_seconds, sizeof(s_buffer_seconds), "%S", tick_time);

  // Write the current AM/PM indicator into a buffer
  static char s_buffer_meridiem[4];
  strftime(s_buffer_meridiem, sizeof(s_buffer_meridiem), "%p", tick_time);

  // Write the current date into a buffer
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%b %d", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_time_buffer);

  // Display seconds on the TextLayer
  text_layer_set_text(s_seconds_layer, s_buffer_seconds);

  // Display AM/PM on the TextLayer
  text_layer_set_text(s_meridiem_layer, s_buffer_meridiem);

  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_date_buffer);
}
/* TICK HANDLER */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

/* ARROWS DRAWING PROC */
static void arrows_update_proc(Layer *layer, GContext *ctx) {
  int sub = s_current_seconds % 10;
  for (int i = 0; i < 5; i++) {
    bool is_solid = false;
    if (sub == 0) {
      is_solid = false;
    } else if (sub <= 5) {
      is_solid = (i < sub);
    } else {
      is_solid = (i >= sub - 5);
    }
    GBitmap *bmp = is_solid ? s_arrow_filled_dark_bitmap : s_arrow_dark_bitmap;
    graphics_draw_bitmap_in_rect(ctx, bmp, GRect(i * 15, 0, 16, 24));
  }
}

/* WINDOW LOAD / UNLOAD */
static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GFont
  s_large_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIXEL_DIGIVOLVE_48));
  s_medium_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIXEL_DIGIVOLVE_24));
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIXEL_DIGIVOLVE_16));

  // Create GBitmap
  s_arrow_dark_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ARROW_DARK);
  s_arrow_filled_dark_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ARROW_FILLED_DARK);

  int arrows_width = 4 * 15 + 16;
  int arrows_x = (bounds.size.w - arrows_width) / 2;

  // construct time TextLayer
  s_time_layer = text_layer_create(
      GRect(0, time_textbox_draw_y, bounds.size.w, time_textbox_height));

  // construct meridiem TextLayer
  s_meridiem_layer = text_layer_create(
      GRect(4, time_textbox_draw_y + 60, arrows_x - 4, time_textbox_height / 2));

  // construct arrows Layer
  s_arrows_layer = layer_create(
      GRect(arrows_x, time_textbox_draw_y + 60, arrows_width, 24));
  layer_set_update_proc(s_arrows_layer, arrows_update_proc);

  // construct seconds TextLayer
  s_seconds_layer = text_layer_create(
      GRect(arrows_x + arrows_width, time_textbox_draw_y + 60, bounds.size.w - (arrows_x + arrows_width), time_textbox_height / 2));

  // construct date TextLayer
  s_date_layer = text_layer_create(
      GRect(0, time_textbox_draw_y + 100, bounds.size.w, date_textbox_height));

  // style time TextLayer
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_large_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);

  // style seconds TextLayer
  text_layer_set_background_color(s_seconds_layer, GColorBlack);
  text_layer_set_text_color(s_seconds_layer, GColorWhite);
  text_layer_set_font(s_seconds_layer, s_medium_font);
  text_layer_set_text_alignment(s_seconds_layer, GTextAlignmentRight);

  // style meridiem TextLayer
  text_layer_set_background_color(s_meridiem_layer, GColorBlack);
  text_layer_set_text_color(s_meridiem_layer, GColorWhite);
  text_layer_set_font(s_meridiem_layer, s_small_font);
  text_layer_set_text_alignment(s_meridiem_layer, GTextAlignmentLeft);

  // style date TextLayer
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_small_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  // Add elements to window layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_meridiem_layer));
  layer_add_child(window_layer, s_arrows_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));
  //layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void prv_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_seconds_layer);
  text_layer_destroy(s_meridiem_layer);
  text_layer_destroy(s_date_layer);
  // Unload GFont
  fonts_unload_custom_font(s_large_font);
  fonts_unload_custom_font(s_medium_font);
  fonts_unload_custom_font(s_small_font);
  // Destroy Arrows Layer & Bitmaps
  layer_destroy(s_arrows_layer);
  gbitmap_destroy(s_arrow_dark_bitmap);
  gbitmap_destroy(s_arrow_filled_dark_bitmap);
}

/* INIT / DEINIT */

static void init(void) {
  s_window = window_create();

  //window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  window_stack_push(s_window, animated);

  window_set_background_color(s_window, GColorBlack);

  update_time();
}

static void deinit(void) {
  window_destroy(s_window);
}

/* MAIN program entry point */

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
