#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dvd_app);
LV_IMG_DECLARE(standard)

/* Backlight configuration */
static const struct gpio_dt_spec backlight =
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Screen Dimensions */
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define IMG_WIDTH 66
#define IMG_HEIGHT 66

void main(void) {
  const struct device *display_dev;

  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    LOG_ERR("Device not ready, aborting test");
    return;
  }

  /* Enable Backlight */
  if (device_is_ready(backlight.port)) {
    gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
  } else {
    LOG_ERR("Backlight device not ready");
  }

  /* Create UI */
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

  /* Create Image Object */
  lv_obj_t *img = lv_img_create(scr);
  lv_img_set_src(img, &standard);
  lv_obj_set_size(img, IMG_WIDTH, IMG_HEIGHT);

  /* Initial Position and Velocity */
  int x = 0;
  int y = 0;
  int dx = 2; // Speed X
  int dy = 2; // Speed Y

  lv_obj_set_pos(img, x, y);

  lv_task_handler();
  display_blanking_off(display_dev);

  while (1) {
    /* Update Position */
    x += dx;
    y += dy;

    /* Bounce Logic */
    if (x <= 0) {
      x = 0;
      dx = -dx;
    } else if (x + IMG_WIDTH >= SCREEN_WIDTH) {
      x = SCREEN_WIDTH - IMG_WIDTH;
      dx = -dx;
    }

    if (y <= 0) {
      y = 0;
      dy = -dy;
    } else if (y + IMG_HEIGHT >= SCREEN_HEIGHT) {
      y = SCREEN_HEIGHT - IMG_HEIGHT;
      dy = -dy;
    }

    /* Apply Position */
    lv_obj_set_pos(img, x, y);

    lv_task_handler();
    k_sleep(K_MSEC(20)); // Adjust for speed
  }
}
