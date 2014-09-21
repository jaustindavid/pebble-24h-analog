#include "24h_analog.h"
#include "pebble.h"
#include "string.h"
#include "stdlib.h"

// #define DEBUG
#ifdef DEBUG
  #define TIME_UNIT SECOND_UNIT
#else
  #define TIME_UNIT MINUTE_UNIT
#endif
  
Layer *simple_bg_layer;
Layer *date_layer;
TextLayer *day_label;
char day_buffer[6];
TextLayer *num_label;
char num_buffer[4];
static GPath *hour_arrow;
static GPath *midnight;
static GPath *noon;
Layer *hands_layer;
Window *window;

static void bg_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  //const 
    GPoint center = grect_center_point(&bounds);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);

  graphics_context_set_stroke_color(ctx, GColorWhite);

  gpath_draw_outline(ctx, midnight);
  gpath_draw_filled(ctx, noon);

  center.x -= 1;
  const int16_t faceWidth = bounds.size.w / 2 + 4;
  for (int i = 1; i < 48; i++) {
    GPoint tickOuter, tickInner;
    if (i == 24) {
      continue;
    } 
    const int32_t tick_angle = TRIG_MAX_ANGLE * i / 48;
    tickOuter.x = (int16_t)(sin_lookup(tick_angle) * (int32_t)faceWidth / TRIG_MAX_RATIO) + center.x;
    if (tickOuter.x < 0) tickOuter.x = 0;
    if (tickOuter.x > 143) tickOuter.x = 143;
    tickOuter.y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)faceWidth / TRIG_MAX_RATIO) + center.y;
    if (i % 2 == 1) {
      graphics_draw_pixel(ctx, tickOuter);
    } else {
      int16_t tickLength = 6;
      if (i % 4 == 0) { // even hours == 12
        tickLength += 4;
      } 
      if (i % 12 == 0) { // 6 hours == 16
        tickLength += 8;
      }
      tickInner.x = (int16_t)(sin_lookup(tick_angle) * (int32_t)(faceWidth - tickLength)/ TRIG_MAX_RATIO) + center.x;
      tickInner.y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)(faceWidth - tickLength)/ TRIG_MAX_RATIO) + center.y;
      graphics_draw_line(ctx, tickInner, tickOuter);
    }
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  #ifdef DEBUG
  gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * t->tm_sec / 60));
  #else
  // gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * ((t->tm_hour * 6) + (t->tm_min / 10))) / (24 * 6));
  gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (t->tm_hour + 1.0*t->tm_min/60) / 24));
  #endif
  gpath_draw_filled(ctx, hour_arrow);
  gpath_draw_outline(ctx, hour_arrow);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(day_buffer, sizeof(day_buffer), "%a", t);
  text_layer_set_text(day_label, day_buffer);
  strftime(num_buffer, sizeof(num_buffer), "%d", t);
  text_layer_set_text(num_label, num_buffer);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
  if (tick_time->tm_min == 0) {
    // short pulse on the hour
    vibes_short_pulse();
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // init layers
  simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, simple_bg_layer);
  // init date layer -> a plain parent layer to create a date update proc
  date_layer = layer_create(bounds);
  layer_set_update_proc(date_layer, date_update_proc);
  layer_add_child(window_layer, date_layer);
  // init day
  day_label = text_layer_create(GRect(46, 114, 27, 20));
  text_layer_set_text(day_label, day_buffer);
  text_layer_set_background_color(day_label, GColorBlack);
  text_layer_set_text_color(day_label, GColorWhite);
  GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  text_layer_set_font(day_label, norm18);
  layer_add_child(date_layer, text_layer_get_layer(day_label));
  // init num
  num_label = text_layer_create(GRect(73, 114, 18, 20));
  text_layer_set_text(num_label, num_buffer);
  text_layer_set_background_color(num_label, GColorBlack);
  text_layer_set_text_color(num_label, GColorWhite);
  GFont bold18 = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  text_layer_set_font(num_label, bold18);
  layer_add_child(date_layer, text_layer_get_layer(num_label));
  // init hands
  hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, hands_update_proc);
  layer_add_child(window_layer, hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(simple_bg_layer);
  layer_destroy(date_layer);
  text_layer_destroy(day_label);
  text_layer_destroy(num_label);
  layer_destroy(hands_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  day_buffer[0] = '\0';
  num_buffer[0] = '\0';
  // init hand paths
  // minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  midnight = gpath_create(&MIDNIGHT_POINTS);
  noon = gpath_create(&NOON_POINTS);
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  center.y += 8;
  gpath_move_to(hour_arrow, center);
  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);
  tick_timer_service_subscribe(TIME_UNIT, handle_tick);
}

static void deinit(void) {
  gpath_destroy(hour_arrow);
  gpath_destroy(midnight);
  gpath_destroy(noon);
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}