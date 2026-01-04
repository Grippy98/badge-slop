#include "photos_app.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(badge_launcher);

// Declare external photo assets
LV_IMG_DECLARE(photo_1);
LV_IMG_DECLARE(photo_2);
LV_IMG_DECLARE(photo_3);
LV_IMG_DECLARE(photo_4);
LV_IMG_DECLARE(photo_5);
LV_IMG_DECLARE(photo_6);

static const lv_img_dsc_t *photos[] = {&photo_1, &photo_2, &photo_3,
                                       &photo_4, &photo_5, &photo_6};

#define NUM_PHOTOS (sizeof(photos) / sizeof(photos[0]))

static int current_photo_idx = 0;
static lv_obj_t *img_obj;

// Input State
static int btn_left_prev = 0;
static int btn_right_prev = 0;

static void update_photo(void) {
  if (img_obj) {
    lv_img_set_src(img_obj, photos[current_photo_idx]);

    // Force redraw for E-Ink
    lv_obj_invalidate(lv_scr_act());
  }
}

static void photos_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

  img_obj = lv_img_create(lv_scr_act());
  lv_obj_center(img_obj); // Center on screen (400x300 fills it)

  update_photo();

  // Reset Input
  btn_left_prev = gpio_pin_get_dt(&btn_left);
  btn_right_prev = gpio_pin_get_dt(&btn_right);
}

static void photos_update(void) {
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);

  bool changed = false;

  // Previous Photo
  if (btn_left_curr && !btn_left_prev) {
    current_photo_idx--;
    if (current_photo_idx < 0) {
      current_photo_idx = NUM_PHOTOS - 1;
    }
    changed = true;
    play_beep_move();
  }

  // Next Photo
  if (btn_right_curr && !btn_right_prev) {
    current_photo_idx++;
    if (current_photo_idx >= NUM_PHOTOS) {
      current_photo_idx = 0;
    }
    changed = true;
    play_beep_move();
  }

  if (changed) {
    update_photo();
  }

  btn_left_prev = btn_left_curr;
  btn_right_prev = btn_right_curr;
}

static void photos_exit(void) {
  // Cleanup if needed
}

App photos_app = {.name = "Photos",
                  .enter = photos_enter,
                  .update = photos_update,
                  .exit = photos_exit};
