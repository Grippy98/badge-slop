#include "timer_app.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(timer_app);

static lv_obj_t *time_label;
static lv_obj_t *status_label;
static lv_obj_t *hint_label;

static int64_t total_elapsed_ms = 0;
static int64_t last_tick = 0;
static bool running = false;

// Button state
// defined in app_shared.h (extern)
// static const struct gpio_dt_spec btn_up = GPIO_DT_SPEC_GET(DT_ALIAS(btn_up),
// gpios);
// ... removed ...

static void update_display(void) {
  int s = (total_elapsed_ms / 1000) % 60;
  int m = (total_elapsed_ms / 60000) % 60;
  int h = (total_elapsed_ms / 3600000);

  // Always show HH:MM:SS to prevent jitter
  lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", h, m, s);

  if (running) {
    lv_label_set_text(status_label, "Running");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_GREEN),
                                0);
  } else {
    lv_label_set_text(status_label, "Paused");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_RED),
                                0);
  }
}

static void timer_enter(void) {
  lv_obj_clean(lv_scr_act());

  // Main Container
  lv_obj_t *cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Status Label
  status_label = lv_label_create(cont);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);

  // Time Label
  time_label = lv_label_create(cont);
  // Use a large font or scale it appropriately
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);

  // Hints
  hint_label = lv_label_create(cont);
  lv_label_set_text(hint_label,
                    "SELECT: Start/Stop\nUP/DOWN: Reset\nLEFT: Exit");
  lv_obj_set_style_text_align(hint_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(hint_label, &lv_font_montserrat_14, 0);

  // Reset state on entry?
  // User might want to keep it running in background, but standard app behavior
  // usually resets or resumes. Let's resume if it was running?
  // Actually, for a simple app refactor, let's just re-init variables if you
  // want a fresh start. But usually timers are useful if they persist. Let's
  // keep state global but reset last_tick.
  last_tick = k_uptime_get();
  update_display();
}

static void timer_update(void) {
  static int select_prev = 0;
  static int up_prev = 0;
  static int down_prev = 0;

  int select = gpio_pin_get_dt(&btn_select);
  int up = gpio_pin_get_dt(&btn_up);
  int down = gpio_pin_get_dt(&btn_down);
  int left = gpio_pin_get_dt(&btn_left);

  // EXIT
  if (left) { // Exit immediately on left press
    // State is preserved in static vars if we come back
    return_to_menu();
    return;
  }

  // TOGGLE (Select)
  if (select && !select_prev) {
    running = !running;
    last_tick = k_uptime_get(); // Reset tick delta reference
    update_display();
  }

  // RESET (Up or Down)
  if ((up && !up_prev) || (down && !down_prev)) {
    total_elapsed_ms = 0;
    running = false; // Usually reset stops it too? Or just resets count loops?
                     // "Clears it" implies 0. Let's Stop it too.
    update_display();
  }

  select_prev = select;
  up_prev = up;
  down_prev = down;

  // Time Accumulation
  int64_t now = k_uptime_get();
  if (running) {
    int64_t delta = now - last_tick;
    total_elapsed_ms += delta;

    // Update display every ~50ms for responsiveness
    // But we call update_display() every update loop (which is fast?)
    // We should throttle display updates if loop is super fast,
    // but typical LVGL loop is fine.
    update_display();
  }
  last_tick = now;
}

static void timer_exit(void) {
  // Nothing special to clean up
}

App timer_app = {.name = "Timer",
                 .enter = timer_enter,
                 .update = timer_update,
                 .exit = timer_exit};
