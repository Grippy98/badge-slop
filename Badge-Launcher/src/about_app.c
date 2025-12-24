#include "about_app.h"

static lv_obj_t *main_cont;

static void about_enter(void) {
  main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(main_cont, lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *label = lv_label_create(main_cont);
  lv_label_set_text(
      label,
      "This badge is a labor of love by a diverse\n"
      "team of engineers and makers.\n\n"
      "Huge thanks to - in no particular order: \n\n"
      "Andrei Aldea, Andrew Davis, Judith Mendez, Bryan Brattlof, Devarsh "
      "Thakkar, Jonas Wood, Dhruva Gole, Yogesh Hegde, Sebin Francis, Soumya "
      "Tripathy, Vignesh Raghavendra, Praneeth Bajjuri\n\n"
      "And many others at Texas Instruments, BeagleBoard and Seeed Studio\n\n"
      "(Press BACK to Return)");

  lv_obj_set_width(label, LV_PCT(90));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_center(label);
}

static void about_update(void) {
  // Back handling is done by global main loop now
}

static void about_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App about_app = {.name = "About",
                 .enter = about_enter,
                 .update = about_update,
                 .exit = about_exit};
