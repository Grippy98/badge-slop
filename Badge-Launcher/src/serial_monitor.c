#include "serial_monitor.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

// UART Device (Console)
static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

// Buffer
#define RING_BUF_SIZE 1024
static uint8_t ring_buffer[RING_BUF_SIZE];
static struct ring_buf ringbuf;

// UI
static lv_obj_t *main_cont;
static lv_obj_t *ta;
static lv_obj_t *status_label;
static char temp_read_buf[64];

// UART Callback
static void serial_cb(const struct device *dev, void *user_data) {
  if (!uart_irq_update(dev)) {
    return;
  }

  if (uart_irq_rx_ready(dev)) {
    uint8_t c;
    while (uart_fifo_read(dev, &c, 1) == 1) {
      ring_buf_put(&ringbuf, &c, 1);
    }
  }
}

static void serial_monitor_enter(void) {
  // UI Setup
  main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(main_cont, 5, 0);
  lv_obj_set_style_pad_gap(main_cont, 2, 0);

  // Status Label
  status_label = lv_label_create(main_cont);
  lv_label_set_text(status_label, "- WARNING Shell Disabled -");
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);

  // Text Area
  ta = lv_textarea_create(main_cont);
  lv_obj_set_width(ta, LV_PCT(100));
  lv_obj_set_flex_grow(ta, 1); // Take all remaining space
  lv_textarea_set_text(ta, "");
  lv_textarea_set_max_length(ta, 2000); // Limit history
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);
  lv_obj_set_style_radius(ta, 0, 0); // Sharp corners for E-Ink

  // Setup Ring Buffer
  ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);

  // Hijack UART
  if (!device_is_ready(uart_dev)) {
    lv_textarea_set_text(ta, "Error: UART not ready");
    return;
  }

  // Set callback (This breaks the shell!)
  uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
  uart_irq_rx_enable(uart_dev);
}

static void serial_monitor_update(void) {
  // Poll Buffer
  if (ta) {
    uint8_t *data;
    uint32_t len;

    // Peek/Read from Ring Buffer
    // We can read chunks
    len = ring_buf_get(&ringbuf, (uint8_t *)temp_read_buf,
                       sizeof(temp_read_buf) - 1);
    if (len > 0) {
      temp_read_buf[len] = '\0'; // Null terminate
      lv_textarea_add_text(ta, temp_read_buf);
    }
  }
}

static void serial_monitor_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
    ta = NULL;
  }
  // Note: We DO NOT restore the original callback because we don't have it.
  // Shell remains broken until reboot.
  // We optionally could disable RX to stop interrupts:
  // uart_irq_rx_disable(uart_dev);
}

App serial_monitor_app = {.name = "Serial Monitor",
                          .enter = serial_monitor_enter,
                          .update = serial_monitor_update,
                          .exit = serial_monitor_exit};
