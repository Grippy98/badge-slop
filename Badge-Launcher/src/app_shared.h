#ifndef APP_SHARED_H
#define APP_SHARED_H

#include <lvgl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* App Interface */
typedef struct {
  const char *name;
  void (*enter)(void);
  void (*update)(void);
  void (*exit)(void);
} App;

/* Hardware Definitions (Available to all apps) */
extern const struct gpio_dt_spec btn_up;
extern const struct gpio_dt_spec btn_down;
extern const struct gpio_dt_spec btn_left;
extern const struct gpio_dt_spec btn_right;
extern const struct gpio_dt_spec btn_select;
extern const struct gpio_dt_spec btn_back;

extern const struct gpio_dt_spec led_red;
extern const struct gpio_dt_spec led_green;
extern const struct gpio_dt_spec led_blue;

extern const struct gpio_dt_spec buzzer;

/* Helper Functions */
void play_beep_move(void);
void play_beep_eat(void);
void play_beep_die(void);

void return_to_menu(void);

#endif
