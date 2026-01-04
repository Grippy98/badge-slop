#include "badge_mode_app.h"
#include <zephyr/drivers/display.h>

LOG_MODULE_DECLARE(badge_launcher);

// Declare the converted image asset
LV_IMG_DECLARE(badge_andrei);
LV_IMG_DECLARE(sonia);
LV_IMG_DECLARE(roland);

// Array of badge images - extensible
static const lv_img_dsc_t *badge_images[] = {
    &sonia,
    &roland,
    &badge_andrei,
};
#define NUM_BADGE_IMAGES (sizeof(badge_images) / sizeof(badge_images[0]))

static int current_image_idx = 0;
static bool is_locked = false;

static lv_obj_t *img_obj;
static lv_obj_t *lock_label;

static void update_image(void) {
  if (img_obj) {
    lv_img_set_src(img_obj, badge_images[current_image_idx]);
  }
}

static void update_lock_indicator(void) {
  if (lock_label) {
    if (is_locked) {
      lv_obj_clear_flag(lock_label, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(lock_label, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void badge_enter(void) {
  lv_obj_clean(lv_scr_act());
  // Ensure solid white background
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

  // 1. Full Screen Image
  img_obj = lv_img_create(lv_scr_act());
  lv_obj_center(img_obj); // Center it (400x300 should fill screen)
  update_image();

  // 2. Lock Indicator "L"
  lock_label = lv_label_create(lv_scr_act());
  lv_label_set_text(lock_label, "L");
  lv_obj_set_style_text_font(lock_label, &lv_font_montserrat_14,
                             0); // Small indicator
  lv_obj_set_style_text_color(lock_label, lv_color_black(), 0);
  lv_obj_set_style_bg_color(lock_label, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(lock_label, LV_OPA_COVER,
                          0); // Opaque background to be visible over image
  lv_obj_set_style_pad_all(lock_label, 2, 0);
  lv_obj_align(lock_label, LV_ALIGN_TOP_LEFT, 5, 5); // Top Left corner

  update_lock_indicator();
}

static void badge_update(void) {
  static int btn_left_prev = 0;
  static int btn_right_prev = 0;
  static int btn_select_prev = 0;

  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);
  int btn_select_curr = gpio_pin_get_dt(&btn_select);

  // --- Select Button (Toggle Lock) ---
  if (btn_select_curr && !btn_select_prev) {
    is_locked = !is_locked;
    update_lock_indicator();
    // Beep on lock toggle? Maybe minor beep
    // play_beep_move();
  }

  // --- Navigation (Only if Unlocked) ---
  if (!is_locked) {
    // Left
    if (btn_left_curr && !btn_left_prev) {
      current_image_idx--;
      if (current_image_idx < 0)
        current_image_idx = NUM_BADGE_IMAGES - 1;
      update_image();
    }
    // Right
    if (btn_right_curr && !btn_right_prev) {
      current_image_idx++;
      if (current_image_idx >= NUM_BADGE_IMAGES)
        current_image_idx = 0;
      update_image();
    }
  }

  btn_left_prev = btn_left_curr;
  btn_right_prev = btn_right_curr;
  btn_select_prev = btn_select_curr;
}

static void badge_exit(void) {
  // Reset state on exit? Or keep it?
  // Let's keep state so user returns to same image/lock state if they
  // accidentally backed out Actually, usually exit implies cleanup. But static
  // vars persist.
}

App badge_mode_app = {.name = "Badge Mode",
                      .enter = badge_enter,
                      .update = badge_update,
                      .exit = badge_exit};
