#include "dvd_app.h"
#include "app_shared.h"

LOG_MODULE_DECLARE(badge_launcher);

LV_IMG_DECLARE(beagle);
LV_IMG_DECLARE(sleeping);
LV_IMG_DECLARE(dvd);

typedef struct {
  const lv_img_dsc_t *dsc;
  int offset_top;
  int offset_bottom;
} SpriteInfo;

static const SpriteInfo sprites[] = {
    {&beagle, 0, 0},
    {&sleeping, 0, 0},
    {&dvd, 40, 32} // 40px top, 32px bottom padding
};

static int current_sprite_index = 0;
#define NUM_SPRITES (sizeof(sprites) / sizeof(sprites[0]))

static lv_obj_t *img;
static int x, y;
static int dx, dy;
static int32_t display_width;
static int32_t display_height;
static int32_t img_width;
static int32_t img_height;
static int64_t last_tick;

// Input State
static int btn_left_prev = 0;
static int btn_right_prev = 0;
static int btn_up_prev = 0;
static int current_speed = 10;

static void update_sprite_dimensions(void) {
  if (img) {
    const SpriteInfo *info = &sprites[current_sprite_index];
    lv_img_set_src(img, info->dsc);

    img_width = info->dsc->header.w;
    img_height = info->dsc->header.h;

    // Clamp position if we switched to a larger sprite while near the edge
    // We use the full image dimensions for clamping to safe bounds initially
    if (x + img_width > display_width)
      x = display_width - img_width;
    if (y + img_height > display_height)
      y = display_height - img_height;
  }
}

static void dvd_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(lv_scr_act(),
                            LV_SCROLLBAR_MODE_OFF); // Disable scrollbar

  display_width = lv_disp_get_hor_res(NULL);
  display_height = lv_disp_get_ver_res(NULL);

  img = lv_img_create(lv_scr_act());
  // Start with default
  current_sprite_index = 2; // Default to DVD
  update_sprite_dimensions();

  x = 0;
  y = 0; // Start at 0,0 - update loop will handle visual offset correction if
         // needed on first bounce

  // If starting with DVD sprite, adjust Y to be visually at top if it has
  // padding
  const SpriteInfo *info = &sprites[current_sprite_index];
  if (info->offset_top > 0) {
    y = -info->offset_top;
  }

  dx = 10;
  dy = 10;

  last_tick = k_uptime_get();

  // Instructions Label
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "< > : Logo   ^ : Speed");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -5);

  // Reset input state
  btn_left_prev = gpio_pin_get_dt(&btn_left);
  btn_right_prev = gpio_pin_get_dt(&btn_right);
}

static void dvd_update(void) {
  // Input Handling (Run every cycle for responsiveness)
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);
  bool sprite_changed = false;

  if (btn_left_curr && !btn_left_prev) {
    current_sprite_index--;
    if (current_sprite_index < 0)
      current_sprite_index = NUM_SPRITES - 1;
    sprite_changed = true;
    play_beep_move();
  }

  if (btn_right_curr && !btn_right_prev) {
    current_sprite_index++;
    if (current_sprite_index >= NUM_SPRITES)
      current_sprite_index = 0;
    sprite_changed = true;
    play_beep_move();
  }

  btn_left_prev = btn_left_curr;
  btn_right_prev = btn_right_curr;

  if (sprite_changed) {
    update_sprite_dimensions();
    lv_obj_invalidate(lv_scr_act()); // Force redraw immediately
  }

  // Animation Logic (Throttled)
  // Animation Logic (Throttled)
  int64_t now = k_uptime_get();
  if (now - last_tick > 500) {

    // Speed Control (UP Button) - Increment permanent speed
    int btn_up_curr = gpio_pin_get_dt(&btn_up);
    if (btn_up_curr && !btn_up_prev) {
      current_speed += 10;
      if (current_speed > 50)
        current_speed = 10; // Cycle 10-50
      play_beep_move();     // Audio feedback
    }
    btn_up_prev = btn_up_curr;

    // Preserve direction but apply new speed magnitude
    dx = (dx > 0) ? current_speed : -current_speed;
    dy = (dy > 0) ? current_speed : -current_speed;

    lv_obj_set_pos(img, x, y);

    /* Force Full Redraw for E-Ink artifact cleaning */
    lv_obj_invalidate(lv_scr_act());

    last_tick = now;

    /* Update Position */
    x += dx;
    y += dy;

    /* Bounce Logic */
    const SpriteInfo *info = &sprites[current_sprite_index];
    int offset_top = info->offset_top;
    int offset_bottom = info->offset_bottom;

    // Horizontal Bounce (Standard)
    if (x <= 0) {
      x = 0;
      dx = -dx;
    } else if (x + img_width >= display_width) {
      x = display_width - img_width;
      dx = -dx;
    }

    // Vertical Bounce (with Offsets)
    // Top collision: visual top is (y + offset_top)
    if (y + offset_top <= 0) {
      y = -offset_top; // Snap so visual top is at 0
      dy = -dy;
    }
    // Bottom collision: visual bottom is (y + img_height - offset_bottom)
    else if (y + img_height - offset_bottom >= display_height) {
      y = display_height - (img_height - offset_bottom); // Snap
      dy = -dy;
    }
  }
}

static void dvd_exit(void) {
  // Cleanup
}

App dvd_app = {.name = "DVD Screensaver",
               .enter = dvd_enter,
               .update = dvd_update,
               .exit = dvd_exit};
