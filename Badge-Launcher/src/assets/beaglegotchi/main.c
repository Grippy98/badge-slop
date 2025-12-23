/*
 * Copyright (c) 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logo.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

/* Button configuration */
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(feed_button), gpios);
static struct gpio_callback button0_cb_data;

///* Backlight / LED configuration */
//#define LED0_NODE DT_ALIAS(led0)
//static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Tamagotchi State */
typedef struct
{
	int hunger; // 0-100, 100 is full/not hungry
	bool is_sleeping;
	bool is_eating;
} Tamagotchi;

static Tamagotchi pet = {
	.hunger = 80,
	.is_sleeping = false,
	.is_eating = false,
};

static int inactivity_timer = 0;

/* UI Elements */
static lv_obj_t *hunger_bar;
static lv_obj_t *status_label;

/* Sprite Elements */
static lv_obj_t *face_img;

/* Update UI based on state */
static void update_ui(void)
{
	lv_bar_set_value(hunger_bar, pet.hunger, LV_ANIM_ON);

	/* Face Logic */
	const void *src = &standard;

	if (pet.is_eating) {
		src = &eating;
		lv_label_set_text(status_label, "Yum!");
	} else if (pet.hunger < 33) {
		src = &hungry;
		lv_label_set_text(status_label, "Feed Me!");
	} else if (pet.is_sleeping) {
		src = &sleeping;
		lv_label_set_text(status_label, "Zzz...");
	} else {
		src = &standard;
		lv_label_set_text(status_label, "Happy!");
	}

	lv_img_set_src(face_img, src);
}

/* Game Logic */
static void game_tick(void)
{
	inactivity_timer++;

	// Auto-sleep at 10 seconds of inactivity
	if (inactivity_timer >= 10) {
		pet.is_sleeping = true;
	}

	if (pet.is_sleeping) {
		pet.hunger -= 1; //Gets hungry slower while sleeping
	} else {
		pet.hunger -= 2;
	}

	if (pet.is_eating) {
		pet.is_eating = false; // Eating animation lasts one tick, may want to increase
	}

	// Clamp values
	if (pet.hunger < 0)
		pet.hunger = 0;

	update_ui();
}

/* Button Callbacks */
void button0_pressed(const struct device *dev, struct gpio_callback *cb,
		uint32_t pins)
{
	LOG_INF("Button 0 Pressed");

	// Reset inactivity timer
	inactivity_timer = 0;

	// Feed
	pet.hunger += 20;
	if (pet.hunger > 100)
		pet.hunger = 100;
	pet.is_eating = true;

	// Wake up
	if (pet.is_sleeping) {
		pet.is_sleeping = false;
	}

	update_ui();
}

int main(void)
{
//	const struct device *display_dev;
//	int ret;

//	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
//	if (!device_is_ready(display_dev)) {
//		LOG_ERR("Device not ready");
//		return;
//	}

//  /* Enable Backlight */
//  if (device_is_ready(backlight.port)) {
//    gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
//  } else {
//    LOG_ERR("Backlight device not ready");
//  }

	/* Configure Buttons */
	if (device_is_ready(button0.port)) {
		gpio_pin_configure_dt(&button0, GPIO_INPUT);
		gpio_init_callback(&button0_cb_data, button0_pressed, BIT(button0.pin));
		gpio_add_callback(button0.port, &button0_cb_data);
		gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
	}

	/* Create UI */
	lv_obj_t *scr = lv_scr_act();
	lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

	// Bars on Left
	hunger_bar = lv_bar_create(scr);
	lv_obj_set_size(hunger_bar, 90, 10);
	lv_obj_align(hunger_bar, LV_ALIGN_LEFT_MID, 10, 0); // Centered vertically on left
	lv_bar_set_range(hunger_bar, 0, 100);
	lv_obj_set_style_bg_color(hunger_bar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);

	// Face Image on Right
	face_img = lv_img_create(scr);
	lv_img_set_src(face_img, &standard);
	lv_obj_set_size(face_img, 100, 100);
	lv_obj_align(face_img, LV_ALIGN_RIGHT_MID, -10, 0);
	// Ensure we only see one sprite
	lv_obj_set_style_clip_corner(face_img, true, 0); // Might not be enough for image offset clipping

	// Status Label
	status_label = lv_label_create(scr);
	lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(status_label, lv_color_black(), 0);
	lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);

	lv_task_handler();
//	display_blanking_off(display_dev);

	update_ui();

	while (1) {
		lv_task_handler();

		static int tick_count = 0;
		if (++tick_count >= 100) { // Update stats every 1 second (approx)
			game_tick();
			tick_count = 0;
		}

		k_sleep(K_MSEC(100));
	}
}
