#include "i2c_scanner_app.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(i2c_scanner_app, LOG_LEVEL_INF);

static void i2c_scanner_enter(void);
static void i2c_scanner_update(void);
static void i2c_scanner_exit(void);

// App Structure
App i2c_scanner_app = {.name = "I2C Scanner",
                       .enter = i2c_scanner_enter,
                       .update = i2c_scanner_update,
                       .exit = i2c_scanner_exit};

// --- UI Elements ---
static lv_obj_t *main_cont = NULL;
static lv_obj_t *title_label;
static lv_obj_t *status_label;
static lv_obj_t *result_area;
static lv_obj_t *hint_label;
static lv_obj_t *btn_i2c0;
static lv_obj_t *btn_i2c1;
static lv_obj_t *btn_i2c2;

// --- State ---
typedef enum { STATE_SELECT_BUS, STATE_SCANNING, STATE_RESULTS } ScannerState;

static ScannerState current_state = STATE_SELECT_BUS;
static int selected_bus_index = 0; // 0=I2C0, 1=I2C1, 2=I2C2

static const struct device *i2c_devs[3];
static int64_t last_action_time = 0;

static void init_i2c_devs(void) {
  i2c_devs[0] = DEVICE_DT_GET(DT_ALIAS(i2c0));
  i2c_devs[1] = DEVICE_DT_GET(DT_ALIAS(i2c1));
  i2c_devs[2] = DEVICE_DT_GET(DT_ALIAS(i2c2));
}

// Helper to create a standard button
static lv_obj_t *create_btn(lv_obj_t *parent, const char *text) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 120, 40);
  lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_border_color(btn, lv_color_black(), 0);
  lv_obj_set_style_radius(btn, 0, 0);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  lv_obj_set_style_text_color(label, lv_color_black(), 0);

  return btn;
}

static void highlight_btn(lv_obj_t *btn, bool selected) {
  if (selected) {
    lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_white(), 0);
  } else {
    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_black(), 0);
  }
}

static void scan_bus(int bus_idx) {
  lv_label_set_text(status_label, "Scanning...");
  lv_obj_clean(result_area);
  lv_task_handler(); // Force update UI before blocking

  if (i2c_devs[0] == NULL)
    init_i2c_devs();
  const struct device *dev = i2c_devs[bus_idx];

  if (!dev || !device_is_ready(dev)) {
    lv_label_set_text(status_label, "Error: Device Not Ready");
    return;
  }

  // Configure explicitly
  int ret = i2c_configure(dev, I2C_SPEED_SET(I2C_SPEED_STANDARD) |
                                   I2C_MODE_CONTROLLER);
  if (ret < 0) {
    LOG_ERR("I2C Configure Failed: %d", ret);
    lv_label_set_text_fmt(status_label, "Config Error: %d", ret);
    return;
  }

  // Grid Container
  lv_obj_t *grid = lv_obj_create(result_area);
  // 16 cols * 17px (15+2 gap) ~= 272 width. 8 rows * 17px ~= 136 height.
  lv_obj_set_size(grid, 280, 150);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_all(grid, 2, 0);
  lv_obj_set_style_pad_gap(grid, 2, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  // Center grid itself in the parent flex (handled by parent align, but good to
  // ensure)
  lv_obj_set_align(grid, LV_ALIGN_CENTER);

  int width = 15;
  int height = 15;
  int count = 0;

  for (int i = 0; i < 128; i++) {
    bool scanned = (i >= 0x03 && i <= 0x77);
    bool found = false;

    if (scanned) {
      struct i2c_msg msgs[1];
      uint8_t dst;
      msgs[0].buf = &dst;
      msgs[0].len = 0;
      msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

      if (i2c_transfer(dev, &msgs[0], 1, i) == 0) {
        found = true;
        count++;
      }
    }

    // Cell
    lv_obj_t *cell = lv_obj_create(grid);
    lv_obj_set_size(cell, width, height);
    lv_obj_set_style_radius(cell, 2, 0);
    lv_obj_set_style_border_width(cell, 1, 0);
    lv_obj_set_style_border_color(cell, lv_color_black(), 0);
    lv_obj_set_style_pad_all(cell, 0, 0);

    if (found) {
      lv_obj_set_style_bg_color(cell, lv_color_black(), 0);
    } else {
      lv_obj_set_style_bg_color(cell, lv_color_white(), 0);
      // Gray out reserved/unscanned
      if (!scanned) {
        lv_obj_set_style_bg_color(cell, lv_color_make(200, 200, 200), 0);
        lv_obj_set_style_border_width(cell, 0, 0);
      }
    }

    // Tooltip/Number (optional, maybe too small)
    // For now simple visual grid
  }

  if (count > 100) {
    lv_label_set_text(status_label, "BUS STUCK LOW!");
  } else {
    lv_label_set_text_fmt(status_label, "Found %d Devices", count);
  }
}

static void i2c_scanner_enter(void) {
  current_state = STATE_SELECT_BUS;
  selected_bus_index = 0;
  last_action_time = k_uptime_get(); // Prevent stale clicks

  main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(main_cont, lv_color_white(), 0);
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *header = lv_obj_create(main_cont);
  lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  title_label = lv_label_create(header);
  lv_label_set_text(title_label, "I2C Scanner");
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);

  status_label = lv_label_create(header);
  lv_label_set_text(status_label, "Select Bus");
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);

  lv_obj_t *btn_cont = lv_obj_create(main_cont);
  lv_obj_set_size(btn_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  btn_i2c0 = create_btn(btn_cont, "I2C 0");
  btn_i2c1 = create_btn(btn_cont, "I2C 1*");
  btn_i2c2 = create_btn(btn_cont, "I2C 2*");

  highlight_btn(btn_i2c0, true);
  highlight_btn(btn_i2c1, false);
  highlight_btn(btn_i2c2, false);

  result_area = lv_obj_create(main_cont);
  lv_obj_set_width(result_area, LV_PCT(100));
  lv_obj_set_flex_grow(result_area, 1);
  lv_obj_set_flex_flow(result_area, LV_FLEX_FLOW_COLUMN);
  // Center Children (Grid) Horizontal & Vertical
  lv_obj_set_flex_align(result_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_color(result_area, lv_color_black(), 0);
  lv_obj_set_style_border_width(result_area, 1, 0);

  // Footer Container (Fixed at bottom)
  lv_obj_t *footer = lv_obj_create(main_cont);
  lv_obj_set_size(footer, LV_PCT(100), 30);
  lv_obj_set_style_bg_color(footer, lv_color_white(), 0); // White Bar
  lv_obj_set_style_pad_all(footer, 0, 0);
  lv_obj_set_style_border_width(footer, 1, 0); // Top border
  lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_radius(footer, 0, 0);
  lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Hint Label inside Footer
  hint_label = lv_label_create(footer);
  lv_label_set_text(hint_label, "");
  lv_obj_set_style_text_color(hint_label, lv_color_black(), 0); // Black Text
}

static void i2c_scanner_update(void) {
  // Read GPIOs
  static const struct gpio_dt_spec btn_up =
      GPIO_DT_SPEC_GET(DT_ALIAS(btn_up), gpios);
  static const struct gpio_dt_spec btn_left =
      GPIO_DT_SPEC_GET(DT_ALIAS(btn_left), gpios);
  static const struct gpio_dt_spec btn_right =
      GPIO_DT_SPEC_GET(DT_ALIAS(btn_right), gpios);
  static const struct gpio_dt_spec btn_select =
      GPIO_DT_SPEC_GET(DT_ALIAS(btn_select), gpios);

  // Debounce / Edge State
  static int up_prev = 0, right_prev = 0, left_prev = 0, select_prev = 0;

  int up = gpio_pin_get_dt(&btn_up);
  int right = gpio_pin_get_dt(&btn_right);
  int left = gpio_pin_get_dt(&btn_left);
  int select = gpio_pin_get_dt(&btn_select);
  int64_t now = k_uptime_get();

  // Basic Debounce (ignore fast repeats)
  if (now - last_action_time < 200) { // 200ms dead time
    up_prev = up;
    right_prev = right;
    left_prev = left;
    select_prev = select;
    return;
  }

  if (current_state == STATE_SELECT_BUS) {
    if (right && !right_prev) {
      selected_bus_index++;
      if (selected_bus_index > 2)
        selected_bus_index = 0;
      last_action_time = now;
    }

    if (left && !left_prev) {
      selected_bus_index--;
      if (selected_bus_index < 0)
        selected_bus_index = 2;
      last_action_time = now;
    }

    // Highlight logic
    highlight_btn(btn_i2c0, selected_bus_index == 0);
    highlight_btn(btn_i2c1, selected_bus_index == 1);
    highlight_btn(btn_i2c2, selected_bus_index == 2);

    // Confirm Selection - UP or SELECT BUTTON
    if ((up && !up_prev) || (select && !select_prev)) {
      current_state = STATE_SCANNING;
      last_action_time = now;
      scan_bus(selected_bus_index);
      current_state = STATE_RESULTS;

      lv_label_set_text(hint_label, "Press LEFT to Return");
    }
  } else if (current_state == STATE_RESULTS) {
    // Return to select - LEFT ONLY
    if (left && !left_prev) {
      current_state = STATE_SELECT_BUS;
      last_action_time = now;
      lv_label_set_text(status_label, "Select a Bus");
      lv_label_set_text(hint_label, "");
      lv_obj_clean(result_area);
    }
  }

  up_prev = up;
  right_prev = right;
  left_prev = left;
  select_prev = select;
}

static void i2c_scanner_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}
