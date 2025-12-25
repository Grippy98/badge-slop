#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/mem_manage.h>

#include <lvgl.h>

#include "app_shared.h" // For App struct and buttons
#include "doomgeneric.h"

// Doom Keys
#define KEY_RIGHTARROW 0xae
#define KEY_LEFTARROW 0xac
#define KEY_UPARROW 0xad
#define KEY_DOWNARROW 0xaf
#define KEY_FIRE 0xa3
#define KEY_USE 0xa2
#define KEY_ESCAPE 27
#define KEY_ENTER 13

LOG_MODULE_REGISTER(doom_app);

// Use a fixed resolution for Doom logic
#ifndef DOOMGENERIC_RESX
#define DOOMGENERIC_RESX 320
#endif
#ifndef DOOMGENERIC_RESY
#define DOOMGENERIC_RESY 200
#endif

// LVGL Object for display
static lv_obj_t *doom_canvas_obj;
static lv_obj_t *doom_scr;

// Canvas Buffer
// LVGL 9: Use LV_COLOR_FORMAT_RGB565
// 400 * 300 * 2 = 240KB (Full Screen Buffer)
// We draw 320x200 centered into this.
static uint8_t lvgl_fb[400 * 300 * 2]; // RGB565
static lv_image_dsc_t doom_img_dsc;

static bool doom_initialized = false;

// Button Mapping
static const struct {
  const struct gpio_dt_spec *btn;
  unsigned char key_code;
} key_map[] = {
    {&btn_up, KEY_UPARROW},
    {&btn_down, KEY_DOWNARROW},
    {&btn_left, KEY_LEFTARROW},
    {&btn_right, KEY_RIGHTARROW},
    {&btn_select, KEY_FIRE} // Select = Fire
};
static int btn_states[5] = {0};
static int scan_idx = 0;

static void doom_enter(void) {
  // Map Doom Memory Region (manually reserved in DTS)
  uintptr_t doom_phys = 0x87000000;
  size_t doom_size = 6 * 1024 * 1024;
  uint32_t flags = K_MEM_CACHE_WB | K_MEM_PERM_RW;

  mm_reg_t virt_addr;
  device_map(&virt_addr, doom_phys, doom_size, flags);

  extern volatile uintptr_t doom_virt_addr;
  doom_virt_addr = (uintptr_t)virt_addr;

  printk("DOOM: Mapped 0x%lx to 0x%lx\n", doom_phys, doom_virt_addr);

  doom_scr = lv_obj_create(NULL);
  lv_scr_load(doom_scr);

  // Create Image Object (LVGL 9)
  doom_canvas_obj = lv_image_create(doom_scr);
  lv_obj_center(doom_canvas_obj);

  // Setup Image Descriptor (LVGL 9)
  memset(&doom_img_dsc, 0, sizeof(doom_img_dsc));

  // Clear the framebuffer to black (for letterboxing)
  memset(lvgl_fb, 0, sizeof(lvgl_fb));

#ifdef LV_IMAGE_HEADER_MAGIC
  doom_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
#endif

  // The LVGL Image is the full 400x300 screen
  doom_img_dsc.header.w = 400;
  doom_img_dsc.header.h = 300;
  doom_img_dsc.data_size = sizeof(lvgl_fb);

  doom_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  doom_img_dsc.header.stride = 400 * 2; // 800 bytes

  doom_img_dsc.data = lvgl_fb;

  lv_image_set_src(doom_canvas_obj, &doom_img_dsc);

  if (!doom_initialized) {
    LOG_INF("Initializing Doom...");

    char *argv[] = {"doom", "-nosound", "-nomusic", "-iwad", "doom1.wad"};
    doomgeneric_Create(5, argv);
    doom_initialized = true;
  }
}

static void doom_update(void) {
  if (!doom_initialized)
    return;

  // Run one tick of Doom (game logic)
  doomgeneric_Tick();

  // Back button handled by main loop
}

static void doom_exit(void) {
  // Cleanup if needed
}

// --- DoomGeneric Interface Implementation ---

uint32_t DG_GetTicksMs() { return (uint32_t)k_uptime_get(); }

void DG_SleepMs(uint32_t ms) { k_sleep(K_MSEC(ms)); }

void DG_Init() { LOG_INF("DG_Init Called"); }

int DG_GetKey(int *pressed, unsigned char *key) {
  int count = sizeof(key_map) / sizeof(key_map[0]);

  while (scan_idx < count) {
    int current = gpio_pin_get_dt(key_map[scan_idx].btn);
    if (current != btn_states[scan_idx]) {
      *pressed = current;
      *key = key_map[scan_idx].key_code;
      btn_states[scan_idx] = current;
      scan_idx++;
      return 1; // Event converted
    }
    scan_idx++;
  }

  scan_idx = 0; // Reset for next tick
  return 0;     // No more events
}

void DG_DrawFrame() {
  // Convert ARGB8888 (DG_ScreenBuffer) to RGB565 (lvgl_fb)
  // Doom Resolution: 320x200
  // Screen Resolution: 400x300
  // Centering Offsets: X=40, Y=50

  uint32_t *src = (uint32_t *)DG_ScreenBuffer;
  uint16_t *dst_base = (uint16_t *)lvgl_fb;

  // Clear background (optional, but good for safety)
  // memset(lvgl_fb, 0, sizeof(lvgl_fb));

  int x_offset = (400 - DOOMGENERIC_RESX) / 2;
  int y_offset = (300 - DOOMGENERIC_RESY) / 2;
  int dst_stride = 400; // Width of the physical screen buffer

  for (int y = 0; y < DOOMGENERIC_RESY; y++) {
    for (int x = 0; x < DOOMGENERIC_RESX; x++) {
      uint32_t p = src[y * DOOMGENERIC_RESX + x];
      uint8_t r = (p >> 16) & 0xFF;
      uint8_t g = (p >> 8) & 0xFF;
      uint8_t b = p & 0xFF;

      // Brightness Boost (Simple Scalar)
      // Doom is dark. E-Ink needs contrast/brightness.
      // Scale by 1.5x (approx)
      unsigned int r_boost = (unsigned int)r * 3 / 2;
      unsigned int g_boost = (unsigned int)g * 3 / 2;
      unsigned int b_boost = (unsigned int)b * 3 / 2;

      if (r_boost > 255)
        r_boost = 255;
      if (g_boost > 255)
        g_boost = 255;
      if (b_boost > 255)
        b_boost = 255;

      r = (uint8_t)r_boost;
      g = (uint8_t)g_boost;
      b = (uint8_t)b_boost;

      // RGB565
      uint16_t c = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

      // Calculate destination index with centering
      int dst_idx = (y + y_offset) * dst_stride + (x + x_offset);
      dst_base[dst_idx] = c;
    }
  }

  lv_obj_invalidate(doom_canvas_obj);
}

void DG_SetWindowTitle(const char *title) {}

App doom_app = {.name = "DOOM",
                .enter = doom_enter,
                .update = doom_update,
                .exit = doom_exit};
