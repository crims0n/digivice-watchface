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
static GPath *s_triangle_path;
static int s_current_seconds = 0;

static const GPathInfo TRIANGLE_POINTS = {
  .num_points = 3,
  .points = (GPoint []) {
    {0, 0},
    {18, 18},
    {0, 36}
  }
};

static Layer *s_battery_layer;
static int s_battery_level = 100;
static bool s_battery_charging = false;

const int MAX_SCREEN_X = 144;
const int MAX_SCREEN_Y = 168;

const int time_textbox_height = 50;
const int time_textbox_draw_y = 18;
const int seconds_textbox_height = 28;
const int seconds_textbox_draw_y = 66;
const int arrows_draw_y = 104;
const int date_textbox_height = 40;
const int date_textbox_draw_y = 136;

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  s_battery_charging = state.is_charging;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
}

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

  // Write the current AM/PM indicator into a buffer (only in 12h mode)
  static char s_buffer_meridiem[4];
  if (clock_is_24h_style()) {
    s_buffer_meridiem[0] = '\0';
  } else {
    strftime(s_buffer_meridiem, sizeof(s_buffer_meridiem), "%p", tick_time);
  }

  // Write the current date into a buffer
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%b %d", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_time_buffer);

  // Align seconds so they right-justify to the right edge of the centered minutes
  if (s_window && s_seconds_layer && s_large_font) {
    Layer *window_layer = window_get_root_layer(s_window);
    GRect window_bounds = layer_get_bounds(window_layer);
    GSize time_size = graphics_text_layout_get_content_size(
        s_time_buffer, s_large_font,
        GRect(0, 0, window_bounds.size.w, time_textbox_height),
        GTextOverflowModeWordWrap, GTextAlignmentCenter);
    int time_right_x = (window_bounds.size.w + time_size.w) / 2 - 3;
    layer_set_frame(text_layer_get_layer(s_seconds_layer),
                    GRect(0, seconds_textbox_draw_y, time_right_x, seconds_textbox_height));
  }

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

    int ox = i * 27;

    if (is_solid) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      gpath_move_to(s_triangle_path, GPoint(ox, 0));
      gpath_draw_filled(ctx, s_triangle_path);
    } else {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      gpath_move_to(s_triangle_path, GPoint(ox, 0));
      gpath_draw_outline_open(ctx, s_triangle_path);
      gpath_move_to(s_triangle_path, GPoint(ox + 1, 0));
      gpath_draw_outline_open(ctx, s_triangle_path);
    }
  }
}

/* BATTERY METER DRAWING PROC */
static void battery_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);

  // Battery outer body: 20x12
  graphics_draw_rect(ctx, GRect(0, 0, 20, 12));

  // Battery terminal nub on the right (2x6 centered vertically)
  graphics_fill_rect(ctx, GRect(20, 3, 2, 6), 0, GCornerNone);

  // Battery fill bar inside: 16x8 available
  int fill_w = (16 * s_battery_level) / 100;
  if (fill_w > 0) {
    graphics_fill_rect(ctx, GRect(2, 2, fill_w, 8), 0, GCornerNone);
  }

  // Charging indicator
  if (s_battery_charging) {
    graphics_draw_line(ctx, GPoint(10, 3), GPoint(10, 8));
    graphics_draw_line(ctx, GPoint(8, 5), GPoint(12, 5));
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

  // Create GPath
  s_triangle_path = gpath_create(&TRIANGLE_POINTS);

  int arrows_width = 4 * 27 + 20;
  int arrows_x = (bounds.size.w - arrows_width) / 2;
  int battery_width = 24;

  // Top Left: Meridiem TextLayer (AM/PM)
  s_meridiem_layer = text_layer_create(
      GRect(6, 0, 40, 20));

  // Top Right: Battery Meter Layer (icon only)
  s_battery_layer = layer_create(
      GRect(bounds.size.w - battery_width - 6, 5, battery_width, 12));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Middle: Time TextLayer (Centered)
  s_time_layer = text_layer_create(
      GRect(0, time_textbox_draw_y, bounds.size.w, time_textbox_height));

  // Below Time: Seconds TextLayer (right-justified to minutes)
  s_seconds_layer = text_layer_create(
      GRect(0, seconds_textbox_draw_y, bounds.size.w, seconds_textbox_height));

  // Below Seconds: Arrows Layer (Centered, lower)
  s_arrows_layer = layer_create(
      GRect(arrows_x, arrows_draw_y, arrows_width, 38));
  layer_set_update_proc(s_arrows_layer, arrows_update_proc);

  // Construct date TextLayer
  s_date_layer = text_layer_create(
      GRect(0, date_textbox_draw_y, bounds.size.w, date_textbox_height));

  // Style time TextLayer
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_large_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Style seconds TextLayer
  text_layer_set_background_color(s_seconds_layer, GColorBlack);
  text_layer_set_text_color(s_seconds_layer, GColorWhite);
  text_layer_set_font(s_seconds_layer, s_medium_font);
  text_layer_set_text_alignment(s_seconds_layer, GTextAlignmentRight);

  // Style meridiem TextLayer
  text_layer_set_background_color(s_meridiem_layer, GColorBlack);
  text_layer_set_text_color(s_meridiem_layer, GColorWhite);
  text_layer_set_font(s_meridiem_layer, s_small_font);
  text_layer_set_text_alignment(s_meridiem_layer, GTextAlignmentLeft);

  // Style date TextLayer
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_small_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);

  // Add elements to window layer
  layer_add_child(window_layer, text_layer_get_layer(s_meridiem_layer));
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_seconds_layer));
  layer_add_child(window_layer, s_arrows_layer);
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
  // Destroy Layers & Paths
  layer_destroy(s_arrows_layer);
  layer_destroy(s_battery_layer);
  gpath_destroy(s_triangle_path);
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

  // Register with BatteryStateService
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());

  window_stack_push(s_window, animated);

  window_set_background_color(s_window, GColorBlack);

  update_time();
}

static void deinit(void) {
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

/* MAIN program entry point */

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
