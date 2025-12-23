#include "button_test.h"

LOG_MODULE_DECLARE(badge_launcher);

/* UI Elements */
static lv_obj_t *label_up;
static lv_obj_t *label_down;
static lv_obj_t *label_left;
static lv_obj_t *label_right;
static lv_obj_t *label_select;
static lv_obj_t *label_back;

/* State */
static bool state_up = false;
static bool state_down = false;
static bool state_left = false;
static bool state_right = false;
static bool state_select = false;
static bool state_back_btn = false;

static void update_label(lv_obj_t *label, bool pressed) {
  if (pressed) {
    lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_UNDERLINE, 0);
    lv_obj_set_style_text_color(label, lv_color_make(0, 0, 0), 0); // Black
  } else {
    lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, 0);
    lv_obj_set_style_text_color(label, lv_color_make(100, 100, 100), 0); // Grey
  }
}

static void update_ui(void) {
  update_label(label_up, state_up);
  update_label(label_down, state_down);
  update_label(label_left, state_left);
  update_label(label_right, state_right);
  update_label(label_select, state_select);
  update_label(label_back, state_back_btn);
}

static void button_test_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

  lv_obj_t *title = lv_label_create(lv_scr_act());
  lv_label_set_text(title, "Button Test");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  // UP
  label_up = lv_label_create(lv_scr_act());
  lv_label_set_text(label_up, "UP");
  lv_obj_align(label_up, LV_ALIGN_TOP_MID, 0, 30);

  // DOWN
  label_down = lv_label_create(lv_scr_act());
  lv_label_set_text(label_down, "DOWN");
  lv_obj_align(label_down, LV_ALIGN_BOTTOM_MID, 0, -30);

  // LEFT
  label_left = lv_label_create(lv_scr_act());
  lv_label_set_text(label_left, "LEFT");
  lv_obj_align(label_left, LV_ALIGN_LEFT_MID, 20, 0);

  // RIGHT
  label_right = lv_label_create(lv_scr_act());
  lv_label_set_text(label_right, "RIGHT");
  lv_obj_align(label_right, LV_ALIGN_RIGHT_MID, -20, 0);

  // SELECT
  label_select = lv_label_create(lv_scr_act());
  lv_label_set_text(label_select, "SELECT");
  lv_obj_align(label_select, LV_ALIGN_CENTER, 0, 0);

  // BACK
  label_back = lv_label_create(lv_scr_act());
  lv_label_set_text(label_back, "BACK");
  lv_obj_align(label_back, LV_ALIGN_TOP_LEFT, 5, 5);

  // Run Startup LED Sequence
  if (gpio_is_ready_dt(&led_red)) {
    gpio_pin_set_dt(&led_red, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&led_red, 0);
  }
  if (gpio_is_ready_dt(&led_green)) {
    gpio_pin_set_dt(&led_green, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&led_green, 0);
  }
  if (gpio_is_ready_dt(&led_blue)) {
    gpio_pin_set_dt(&led_blue, 1);
    k_sleep(K_MSEC(200));
    gpio_pin_set_dt(&led_blue, 0);
  }
}

/* Debounce Logic */
#define DEBOUNCE_MS 50
static int64_t last_change_time[6]; // Up, Down, Left, Right, Select, Back
static bool stable_states[6];

// Indices
enum {
  BTN_IDX_UP = 0,
  BTN_IDX_DOWN,
  BTN_IDX_LEFT,
  BTN_IDX_RIGHT,
  BTN_IDX_SELECT,
  BTN_IDX_BACK
};

static void handle_button_change(int btn_idx, bool pressed) {
  // UI Updates
  switch (btn_idx) {
  case BTN_IDX_UP:
    update_label(label_up, pressed);
    break;
  case BTN_IDX_DOWN:
    update_label(label_down, pressed);
    break;
  case BTN_IDX_LEFT:
    update_label(label_left, pressed);
    break;
  case BTN_IDX_RIGHT:
    update_label(label_right, pressed);
    break;
  case BTN_IDX_SELECT:
    update_label(label_select, pressed);
    break;
  case BTN_IDX_BACK:
    update_label(label_back, pressed);
    break;
  }

  // LED Updates (While Held)
  if (btn_idx == BTN_IDX_UP && gpio_is_ready_dt(&led_green)) {
    gpio_pin_set_dt(&led_green, pressed);
  }
  if (btn_idx == BTN_IDX_LEFT && gpio_is_ready_dt(&led_red)) {
    gpio_pin_set_dt(&led_red, pressed);
  }
  if (btn_idx == BTN_IDX_RIGHT && gpio_is_ready_dt(&led_blue)) {
    gpio_pin_set_dt(&led_blue, pressed);
  }

  // Beep (Only on Press)
  if (pressed) {
    if (btn_idx == BTN_IDX_LEFT)
      play_beep_move(); // Use shared sounds
    else if (btn_idx == BTN_IDX_RIGHT)
      play_beep_move();
    else
      play_beep_eat(); // Default sound
  }
}

static void process_button(int btn_idx, const struct gpio_dt_spec *spec) {
  if (!spec)
    return;

  int raw_state = gpio_pin_get_dt(spec);
  int64_t now = k_uptime_get();

  if (raw_state != stable_states[btn_idx]) {
    if (now - last_change_time[btn_idx] > DEBOUNCE_MS) {
      stable_states[btn_idx] = raw_state;
      last_change_time[btn_idx] = now;
      handle_button_change(btn_idx, raw_state);
    }
  } else {
    // Reset timer if signal matches stable state (glitch rejection)
    // Actually, for simple debounce, we just wait for stable transition.
    // If it bounces back, the next check will catch it if > 50ms.
    // A more robust way is to update 'last_change' on ANY raw change,
    // effectively requiring signal stability.
    // But simple timeout is usually fine for buttons.
  }
}

static void button_test_update(void) {
  process_button(BTN_IDX_UP, &btn_up);
  process_button(BTN_IDX_DOWN, &btn_down);
  process_button(BTN_IDX_LEFT, &btn_left);
  process_button(BTN_IDX_RIGHT, &btn_right);
  process_button(BTN_IDX_SELECT, &btn_select);
  process_button(BTN_IDX_BACK, &btn_back);
}

static void button_test_exit(void) {
  // Turn off LEDs
  if (gpio_is_ready_dt(&led_red))
    gpio_pin_set_dt(&led_red, 0);
  if (gpio_is_ready_dt(&led_green))
    gpio_pin_set_dt(&led_green, 0);
  if (gpio_is_ready_dt(&led_blue))
    gpio_pin_set_dt(&led_blue, 0);
  if (gpio_is_ready_dt(&buzzer))
    gpio_pin_set_dt(&buzzer, 0);
}

App button_test_app = {.name = "Button Test",
                       .enter = button_test_enter,
                       .update = button_test_update,
                       .exit = button_test_exit};
