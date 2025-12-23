#include "shutdown_app.h"
#include "app_shared.h"

LOG_MODULE_DECLARE(badge_launcher);

// Declare the QR Code image asset
LV_IMG_DECLARE(qr_code);

static void shutdown_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

  // Main Container - Full Screen, Centered Column
  lv_obj_t *cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);

  // Vertical Flex Layout
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(cont, 10, 0); // 10px gap between elements

  // 1. "BeagleBadge" Large Font Label
  lv_obj_t *title_label = lv_label_create(cont);
  lv_label_set_text(title_label, "BeagleBadge");
  // Use the massive 48px font for readability
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(title_label, lv_color_black(), 0);
  lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);

  // 2. "Learn More" Label
  lv_obj_t *sub_label = lv_label_create(cont);
  lv_label_set_text(sub_label, "Learn More");
  // Use the 24px font (previously title size) for subtitle
  lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(sub_label, lv_color_black(), 0);

  // 3. QR Code Image
  lv_obj_t *qr_img = lv_img_create(cont);
  lv_img_set_src(qr_img, &qr_code);
  // Image is 128x128, will be centered by flex align

  // Explicitly invalidate to ensure full draw
  lv_obj_invalidate(lv_scr_act());
}

static void shutdown_update(void) {
  // Static screen
}

static void shutdown_exit(void) {
  // Cleanup handled by lv_obj_clean
}

App shutdown_app = {.name = "Shutdown",
                    .enter = shutdown_enter,
                    .update = shutdown_update,
                    .exit = shutdown_exit};
