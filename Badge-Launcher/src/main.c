#include "about_app.h"
#include "app_shared.h"
#include "badge_mode_app.h"
#include "beagle_man.h"
#include "beagle_run.h"
#include "beaglegotchi.h"
#include "brick_breaker.h"
#include "button_test.h"
#include "dvd_app.h"
#include "froggr.h"
#include "i2c_scanner_app.h"
#include "serial_monitor.h"
#include "shutdown_app.h"
#include "snake_game.h"
#include "space_invaders.h"

#include <zephyr/drivers/display.h>
#include <zephyr/random/random.h>
#include <zephyr/version.h>

LOG_MODULE_REGISTER(badge_launcher);

/* Assets */
LV_IMG_DECLARE(ti_logo);
LV_IMG_DECLARE(beagle);

/* Hardware Definitions */
const struct gpio_dt_spec btn_up = GPIO_DT_SPEC_GET(DT_ALIAS(btn_up), gpios);
const struct gpio_dt_spec btn_down =
    GPIO_DT_SPEC_GET(DT_ALIAS(btn_down), gpios);
const struct gpio_dt_spec btn_left =
    GPIO_DT_SPEC_GET(DT_ALIAS(btn_left), gpios);
const struct gpio_dt_spec btn_right =
    GPIO_DT_SPEC_GET(DT_ALIAS(btn_right), gpios);
const struct gpio_dt_spec btn_select =
    GPIO_DT_SPEC_GET(DT_ALIAS(btn_select), gpios);
const struct gpio_dt_spec btn_back =
    GPIO_DT_SPEC_GET(DT_ALIAS(btn_back), gpios); // Using Back button

/* LEDs */
// Green, Blue or similar. I will use led0, led1, led2.
const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_red), gpios);
const struct gpio_dt_spec led_green =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_green), gpios);
const struct gpio_dt_spec led_blue =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_blue), gpios);

/* Buzzer */
const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);

/* Audio Helpers */
static bool is_muted = false;
void play_beep_move(void) {
  if (is_muted)
    return;
  if (gpio_is_ready_dt(&buzzer)) {
    for (int i = 0; i < 20; i++) {
      gpio_pin_set_dt(&buzzer, 1);
      k_usleep(250);
      gpio_pin_set_dt(&buzzer, 0);
      k_usleep(250);
    }
  }
}
void play_beep_eat(void) {
  if (is_muted)
    return;
  if (gpio_is_ready_dt(&buzzer)) {
    for (int i = 0; i < 50; i++) {
      gpio_pin_set_dt(&buzzer, 1);
      k_usleep(500 - (i * 5));
      gpio_pin_set_dt(&buzzer, 0);
      k_usleep(500 - (i * 5));
    }
  }
}
void play_beep_die(void) {
  if (is_muted)
    return;
  if (gpio_is_ready_dt(&buzzer)) {
    for (int i = 0; i < 100; i++) {
      gpio_pin_set_dt(&buzzer, 1);
      k_usleep(1000 + (i * 20));
      gpio_pin_set_dt(&buzzer, 0);
      k_usleep(1000 + (i * 20));
    }
  }
}

// --- Globals ---
static App *current_app = NULL;
static App *next_app = NULL;
static bool menu_needs_redraw = true;

// --- Mute Toggle App ---
static char mute_app_name[20] = "Sound: ON";

static void update_mute_label(void) {
  if (is_muted) {
    snprintf(mute_app_name, sizeof(mute_app_name), "Sound: OFF");
  } else {
    snprintf(mute_app_name, sizeof(mute_app_name), "Sound: ON");
  }
}

static void mute_app_enter(void) {
  is_muted = !is_muted;
  update_mute_label();
  if (!is_muted) {
    play_beep_move();
  }
}

// Forward declaration of menu_app for return logic
extern App menu_app;
static void mute_app_return(void) { next_app = &menu_app; }

static void mute_app_exit(void) {}

App mute_app = {.name = mute_app_name,
                .enter = mute_app_enter,
                .update = mute_app_return,
                .exit = mute_app_exit};

/* Menu App */
/* Menu App Logic */

// --- Category Arrays ---
// Games
static App *apps_games[] = {
    &beaglegotchi_app,  &snake_game_app, &beagle_run_app, &space_invaders_app,
    &brick_breaker_app, &beagle_man_app, &froggr_app};
#define NUM_GAMES 7

// Apps
static App *apps_apps[] = {&badge_mode_app, &dvd_app};
#define NUM_APPS_CAT 2

// Tools
static App *apps_tools[] = {&i2c_scanner_app, &button_test_app,
                            &serial_monitor_app};
#define NUM_TOOLS 3

// Settings
static App *apps_settings[] = {&mute_app, &about_app, &shutdown_app};
#define NUM_SETTINGS 3

// Root Categories
static const char *category_names[] = {"Apps", "Games", "Tools", "Settings"};
#define NUM_CATEGORIES 4

// --- State ---
typedef enum { MENU_ROOT, MENU_SUBMENU } MenuState;

static MenuState current_state = MENU_ROOT;
static int selected_index = 0; // Index within current view (Category or App)
static int selected_category_index = 0; // Remember selected category
static App **current_app_list = NULL;   // Pointer to current submenu array
static int current_list_count = 0;

static lv_obj_t *menu_list_cont;
static lv_obj_t *arrow_up;
static lv_obj_t *arrow_down;
static lv_obj_t *menu_items[10]; // Max items viewable/cache

static void update_menu_selection(void); // Forward Declaration

static void create_menu_item(lv_obj_t *parent, int index, const char *text) {
  // Container
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, LV_PCT(100),
                  40); // Fill width of parent column, fixed height
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 1, 0); // 1px border by default
  lv_obj_set_style_radius(cont, 0, 0);       // No radius for sharp E-Ink look

  // Label
  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  // Use Montserat 18 for readability
  lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);

  menu_items[index] = cont;
}

static int view_start = 0;
#define MAX_VISIBLE_ITEMS 5

static void update_menu_selection(void) {
  int count =
      (current_state == MENU_ROOT) ? NUM_CATEGORIES : current_list_count;

  // Viewport Count (How many items are actually visible)
  int visible_count = count;
  if (visible_count > MAX_VISIBLE_ITEMS)
    visible_count = MAX_VISIBLE_ITEMS;

  for (int i = 0; i < visible_count; i++) {
    lv_obj_t *cont = menu_items[i];
    if (!cont)
      continue; // Should not happen

    lv_obj_t *label = lv_obj_get_child(cont, 0);

    // Calculate which data index this item represents
    int data_index = view_start + i;

    if (data_index == selected_index) {
      // Selected
      lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
      lv_obj_set_style_border_color(cont, lv_color_black(), 0);
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      // lv_obj_scroll_to_view(cont, LV_ANIM_OFF); // Disabled
    } else {
      // Unselected
      lv_obj_set_style_bg_color(cont, lv_color_white(), 0);
      lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
      lv_obj_set_style_border_color(cont, lv_color_black(), 0);
      lv_obj_set_style_text_color(label, lv_color_black(), 0);
    }
  }

  // Arrows (Using global bounds)
  if (arrow_up) {
    if (selected_index > 0)
      lv_obj_set_style_text_opa(arrow_up, LV_OPA_COVER, 0);
    else
      lv_obj_set_style_text_opa(arrow_up, LV_OPA_TRANSP, 0);
  }
  if (arrow_down) {
    if (selected_index < count - 1)
      lv_obj_set_style_text_opa(arrow_down, LV_OPA_COVER, 0);
    else
      lv_obj_set_style_text_opa(arrow_down, LV_OPA_TRANSP, 0);
  }
}

// Rebuilds list with Viewport Logic
static void rebuild_menu_list(void) {
  if (!menu_list_cont)
    return;

  // Clear array
  for (int i = 0; i < 10; i++)
    menu_items[i] = NULL;
  lv_obj_clean(menu_list_cont);

  int total_count = 0;
  if (current_state == MENU_ROOT) {
    total_count = NUM_CATEGORIES;
  } else {
    total_count = current_list_count;
  }

  // Calculate View Start (Keep Selection Visible)
  if (total_count <= MAX_VISIBLE_ITEMS) {
    view_start = 0;
  } else {
    // Logic: Ensure selected_index is inside [view_start, view_start + MAX-1]
    if (selected_index < view_start) {
      view_start = selected_index;
    } else if (selected_index >= view_start + MAX_VISIBLE_ITEMS) {
      view_start = selected_index - MAX_VISIBLE_ITEMS + 1;
    }

    // Clamp (Should generally be covered by above, but safe to clamp)
    if (view_start < 0)
      view_start = 0;
    if (view_start > total_count - MAX_VISIBLE_ITEMS)
      view_start = total_count - MAX_VISIBLE_ITEMS;
  }

  // Determine Render List Count
  int render_count = total_count;
  if (render_count > MAX_VISIBLE_ITEMS)
    render_count = MAX_VISIBLE_ITEMS;

  for (int i = 0; i < render_count; i++) {
    int data_index = view_start + i;

    // Safety check
    if (data_index >= total_count)
      break;

    const char *text = "";
    if (current_state == MENU_ROOT) {
      text = category_names[data_index];
    } else {
      text = current_app_list[data_index]->name;
    }

    create_menu_item(menu_list_cont, i, text);
  }

  update_menu_selection();
}

static void menu_enter(void) {
  LOG_INF("Entering Menu App. State: %d", current_state);
  update_mute_label(); // Ensure label matches state
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER,
                          0); // Ensure solid background
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

  // Main Flex Container - ROW Layout (Side-by-Side)
  lv_obj_t *main_flex = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_flex, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_border_width(main_flex, 0, 0);
  lv_obj_set_style_pad_all(main_flex, 0, 0);
  lv_obj_set_flex_flow(main_flex, LV_FLEX_FLOW_ROW); // Horizontal
  lv_obj_set_flex_align(main_flex, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_opa(main_flex, LV_OPA_TRANSP, 0);

  // 1. Left Panel (Logo + Title) - 40% Width
  lv_obj_t *left_panel = lv_obj_create(main_flex);
  lv_obj_set_size(left_panel, LV_PCT(40), LV_PCT(100));
  lv_obj_set_style_border_width(left_panel, 0, 0);
  lv_obj_set_style_bg_opa(left_panel, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(left_panel, 10, 0);

  // TI Logo / Beagle Logo (Random)
  lv_obj_t *logo = lv_img_create(left_panel);
  if (sys_rand32_get() % 2 == 0) {
    lv_img_set_src(logo, &ti_logo);
  } else {
    lv_img_set_src(logo, &beagle);
  }
  // Assuming TI Logo is small enough? We'll see. If too big, layout adjusts.

  // Title Text (Wrapped)
  lv_obj_t *title = lv_label_create(left_panel);
  lv_label_set_text(title, "Beagle\nBadge");
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(title, lv_color_black(), 0);

  // Zephyr Version
  lv_obj_t *z_version = lv_label_create(left_panel);
  lv_label_set_text(z_version, "Zephyr - " KERNEL_VERSION_STRING);
  lv_obj_set_style_text_align(z_version, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(z_version, &lv_font_montserrat_14, 0);

  // Version Text
  lv_obj_t *version = lv_label_create(left_panel);
  lv_label_set_text(version, "Build - 122325");
  lv_obj_set_style_text_align(version, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(version, &lv_font_montserrat_14,
                             0); // Smaller font

  // Divider Line (Vertical) between panels
  lv_obj_t *line = lv_obj_create(main_flex);
  lv_obj_set_size(line, 2, LV_PCT(90));
  lv_obj_set_style_bg_color(line, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(line, 0, 0);

  // 2. Right Panel (Menu List)
  lv_obj_t *right_panel = lv_obj_create(main_flex);
  lv_obj_set_size(right_panel, LV_PCT(55), LV_PCT(100));
  lv_obj_set_style_border_width(right_panel, 0, 0);
  lv_obj_set_style_bg_opa(right_panel, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(right_panel, 0, 0);
  lv_obj_set_scrollbar_mode(right_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);

  // Arrow Up
  arrow_up = lv_label_create(right_panel);
  lv_label_set_text(arrow_up, LV_SYMBOL_UP); // Or "^"
  lv_obj_set_style_text_font(arrow_up, &lv_font_montserrat_24, 0);
  // Initial Opacity State
  if (selected_index > 0)
    lv_obj_set_style_text_opa(arrow_up, LV_OPA_COVER, 0);
  else
    lv_obj_set_style_text_opa(arrow_up, LV_OPA_TRANSP, 0);

  // Scroll Container
  menu_list_cont = lv_obj_create(right_panel);
  lv_obj_set_width(menu_list_cont, LV_PCT(100));
  // Fixed height: 5 items * 40px + 4 gaps * 5px + 10px padding = 230px.
  // Set to 230px to fit exactly.
  lv_obj_set_height(menu_list_cont, 230);
  lv_obj_set_style_border_width(menu_list_cont, 0, 0);
  lv_obj_set_flex_flow(menu_list_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(menu_list_cont, 5, 0);
  lv_obj_set_style_pad_gap(menu_list_cont, 5, 0);
  lv_obj_set_scrollbar_mode(menu_list_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(menu_list_cont, LV_OBJ_FLAG_SCROLLABLE);

  // Arrow Down
  arrow_down = lv_label_create(right_panel);
  lv_label_set_text(arrow_down, LV_SYMBOL_DOWN); // Or "v"
  lv_obj_set_style_text_font(arrow_down, &lv_font_montserrat_24, 0);
  // Initial Opacity State
  if (selected_index < NUM_CATEGORIES - 1)
    lv_obj_set_style_text_opa(arrow_down, LV_OPA_COVER, 0);
  else
    lv_obj_set_style_text_opa(arrow_down, LV_OPA_TRANSP, 0);

  rebuild_menu_list();
  menu_needs_redraw = true;
}

static void menu_update(void) {

  static int btn_up_prev = 0;
  static int btn_right_prev = 0;
  static int btn_back_prev = 0;
  static int btn_left_prev = 0;
  static int btn_down_prev = 0;
  static int btn_select_prev = 0;

  int btn_up_curr = gpio_pin_get_dt(&btn_up);
  int btn_right_curr = gpio_pin_get_dt(&btn_right); // Select
  int btn_back_curr = gpio_pin_get_dt(&btn_back);
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_down_curr = gpio_pin_get_dt(&btn_down);
  int btn_select_curr = gpio_pin_get_dt(&btn_select);

  bool action_taken = false;
  int current_max =
      (current_state == MENU_ROOT) ? NUM_CATEGORIES : current_list_count;

  // --- UP Button (Cycles Up/Wrap) ---
  if (btn_up_curr && !btn_up_prev) {
    LOG_INF("UP Pressed. Old Index: %d, Max: %d", selected_index, current_max);
    selected_index--;
    if (selected_index < 0)
      selected_index = current_max - 1; // Wrap to bottom
    LOG_INF("New Index: %d", selected_index);
    play_beep_move();

    // Force Rebuild to prevent ghosting/artifacts on E-Ink
    rebuild_menu_list();
    action_taken = true;
  }

  // --- DOWN Button (Cycles Down/Wrap) ---
  if (btn_down_curr && !btn_down_prev) {
    LOG_INF("DOWN Pressed. Old Index: %d, Max: %d", selected_index,
            current_max);
    selected_index++;
    if (selected_index >= current_max)
      selected_index = 0; // Wrap to top
    LOG_INF("New Index: %d", selected_index);
    play_beep_move();

    rebuild_menu_list();
    action_taken = true;
  }

  // --- RIGHT Button (Select/Enter) ---
  if ((btn_right_curr && !btn_right_prev) ||
      (btn_select_curr && !btn_select_prev)) {
    play_beep_eat();
    if (current_state == MENU_ROOT) {
      // ENTER SUBMENU
      selected_category_index = selected_index;
      current_state = MENU_SUBMENU;

      if (selected_index == 0) { // Apps
        current_app_list = apps_apps;
        current_list_count = NUM_APPS_CAT;
      } else if (selected_index == 1) { // Games
        current_app_list = apps_games;
        current_list_count = NUM_GAMES;
      } else if (selected_index == 2) { // Tools
        current_app_list = apps_tools;
        current_list_count = NUM_TOOLS;
      } else { // Settings
        current_app_list = apps_settings;
        current_list_count = NUM_SETTINGS;
      }
      selected_index = 0; // Reset for submenu
      rebuild_menu_list();
    } else {
      // Check for Special 'Actions' like Mute
      if (current_app_list[selected_index] == &mute_app) {
        is_muted = !is_muted;
        update_mute_label();
        if (!is_muted)
          play_beep_move();
        rebuild_menu_list(); // Refresh visuals immediately
      } else {
        // LAUNCH NORMAL APP
        next_app = current_app_list[selected_index];
      }
    }
    action_taken = true;
  }

  // --- BACK Button (Return to Root) ---
  // Using actual Back button OR Left button
  if ((btn_back_curr && !btn_back_prev) || (btn_left_curr && !btn_left_prev)) {
    if (current_state == MENU_SUBMENU) {
      play_beep_move();
      current_state = MENU_ROOT;
      selected_index = selected_category_index; // Restore position
      rebuild_menu_list();
      action_taken = true;
    }
  }

  btn_up_prev = btn_up_curr;
  btn_right_prev = btn_right_curr;
  btn_back_prev = btn_back_curr;
  btn_left_prev = btn_left_curr;
  btn_down_prev = btn_down_curr;
  btn_select_prev = btn_select_curr;

  if (action_taken || menu_needs_redraw) {
    menu_needs_redraw = false;
    // update_menu_selection(); // Rebuild handles this now
    // Invalidate the list container to ensure redraw
    if (menu_list_cont)
      lv_obj_invalidate(menu_list_cont);
  }
}

static void menu_exit(void) {}

App menu_app = {.name = "Menu",
                .enter = menu_enter,
                .update = menu_update,
                .exit = menu_exit};

int main(void) {
  /* Init Drivers */
  gpio_pin_configure_dt(&btn_up, GPIO_INPUT);
  gpio_pin_configure_dt(&btn_down, GPIO_INPUT);
  gpio_pin_configure_dt(&btn_left, GPIO_INPUT);
  gpio_pin_configure_dt(&btn_right, GPIO_INPUT);
  gpio_pin_configure_dt(&btn_select, GPIO_INPUT);
  gpio_pin_configure_dt(&btn_back, GPIO_INPUT);

  // Outputs
  if (gpio_is_ready_dt(&led_red))
    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
  if (gpio_is_ready_dt(&led_green))
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
  if (gpio_is_ready_dt(&led_blue))
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
  if (gpio_is_ready_dt(&buzzer))
    gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_INACTIVE);

  current_app = &menu_app;
  current_app->enter();

  while (1) {
    lv_task_handler();

    // Global Back Logic (Debounced slightly)
    static int64_t last_back_time = 0;
    int64_t now_main = k_uptime_get();
    if (gpio_pin_get_dt(&btn_back) && current_app != &menu_app) {
      if (now_main - last_back_time > 300) {
        next_app = &menu_app;
        last_back_time = now_main;
      }
    }

    if (next_app) {
      if (current_app->exit) {
        current_app->exit();
      }

      // IF returning to MENU, perform full E-Ink Clear (Black -> White)
      // This removes ghosting from previous apps (especially dark ones like
      // Snake/I2C)
      if (next_app == &menu_app) {
        // 1. Black
        lv_obj_t *black_curtain = lv_obj_create(lv_scr_act());
        lv_obj_set_size(black_curtain, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(black_curtain, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(black_curtain, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(black_curtain, 0, 0);
        lv_task_handler();    // Render Black
        k_sleep(K_MSEC(100)); // Wait for E-Ink

        // 2. White (Clear)
        lv_obj_del(black_curtain);
      }

      // Always ensure clean slate
      lv_obj_clean(lv_scr_act());
      lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
      lv_task_handler(); // Render White
      k_sleep(K_MSEC(50));

      current_app = next_app;
      next_app = NULL;
      current_app->enter();
    }

    // Fallback: If no next_app, just update current
    if (current_app) {
      current_app->update();
    }

    k_sleep(K_MSEC(10)); // Prevent CPU hogging
  }
  return 0;
}
