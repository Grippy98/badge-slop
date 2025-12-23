#include "beaglegotchi.h"
#include "logo.h"

LOG_MODULE_DECLARE(badge_launcher);

/* Game Constants */
#define MAX_STAT 100
#define MIN_STAT 0
#define TICK_INTERVAL 1000 // 1 second
#define SLEEP_RECOVERY 5
#define FEED_RECOVERY 20
#define PLAY_COST 10
#define PLAY_RECOVERY 15
#define DECAY_HUNGER 2
#define DECAY_ENERGY 1
#define DECAY_HAPPY 1

/* State */
typedef struct {
  int hunger;    // 0 = Starving, 100 = Full
  int energy;    // 0 = Exhausted, 100 = Energetic
  int happiness; // 0 = Depressed, 100 = Ecstatic
  bool is_sleeping;
  bool is_eating;
  bool is_playing;
} Tamagotchi;

static Tamagotchi pet = {.hunger = 80,
                         .energy = 80,
                         .happiness = 80,
                         .is_sleeping = false,
                         .is_eating = false,
                         .is_playing = false};

static int64_t last_tick;
static int64_t last_input_time;

/* Menu System */
typedef enum { ACTION_FEED, ACTION_PLAY, ACTION_SLEEP, NUM_ACTIONS } ActionType;

static const char *action_names[] = {"FEED", "PLAY", "SLEEP"};
static int selected_action = 0;

/* UI Elements */
static lv_obj_t *img_sprite;
static lv_obj_t *lbl_status;
static lv_obj_t *bar_hunger;
static lv_obj_t *bar_energy;
static lv_obj_t *bar_happy;
static lv_obj_t *menu_container;
static lv_obj_t *menu_labels[NUM_ACTIONS];
static lv_obj_t *lbl_tooltip;

static void update_sprite(void) {
  if (!img_sprite)
    return;

  const void *src = &standard;
  if (pet.is_sleeping)
    src = &sleeping;
  else if (pet.is_eating)
    src = &eating;
  else if (pet.hunger < 30)
    src = &hungry;
  // else if (pet.is_playing) src = &playing; // If we had one

  lv_img_set_src(img_sprite, src);
}

static void update_bars(void) {
  if (bar_hunger)
    lv_bar_set_value(bar_hunger, pet.hunger, LV_ANIM_OFF);
  if (bar_energy)
    lv_bar_set_value(bar_energy, pet.energy, LV_ANIM_OFF);
  if (bar_happy)
    lv_bar_set_value(bar_happy, pet.happiness, LV_ANIM_OFF);
}

static void update_menu(void) {
  for (int i = 0; i < NUM_ACTIONS; i++) {
    if (menu_labels[i]) {
      // Always Black Text
      lv_obj_set_style_text_color(menu_labels[i], lv_color_black(), 0);

      if (i == selected_action) {
        lv_obj_set_style_text_decor(menu_labels[i], LV_TEXT_DECOR_UNDERLINE, 0);
      } else {
        lv_obj_set_style_text_decor(menu_labels[i], LV_TEXT_DECOR_NONE, 0);
      }
    }
  }
}

static void update_status_text(void) {
  if (!lbl_status)
    return;

  if (pet.is_sleeping) {
    lv_label_set_text(lbl_status, "Status: Sleeping");
  } else if (pet.is_eating) {
    lv_label_set_text(lbl_status, "Status: Eating");
  } else if (pet.hunger < 20) {
    lv_label_set_text(lbl_status, "Status: Starving!");
  } else if (pet.energy < 20) {
    lv_label_set_text(lbl_status, "Status: Exhausted");
  } else if (pet.happiness < 20) {
    lv_label_set_text(lbl_status, "Status: Depressed");
  } else {
    lv_label_set_text(lbl_status, "Status: Happy");
  }
}

static void game_tick(void) {
  if (pet.is_eating) {
    // Regenerate while eating
    pet.hunger += FEED_RECOVERY;
    if (pet.hunger >= MAX_STAT) {
      pet.hunger = MAX_STAT;
      pet.is_eating = false; // Stop when full
    }
    // No decay while eating
  } else if (pet.is_sleeping) {
    // Stat Decay / Regen while sleeping
    pet.energy += SLEEP_RECOVERY;
    pet.hunger -= DECAY_HUNGER / 2; // Digest slower
  } else {
    // Normal Decay
    pet.hunger -= DECAY_HUNGER;
    pet.energy -= DECAY_ENERGY;
    pet.happiness -= DECAY_HAPPY;
  }

  // Cap Stats
  if (pet.energy > MAX_STAT)
    pet.energy = MAX_STAT;
  if (pet.hunger > MAX_STAT)
    pet.hunger = MAX_STAT;
  if (pet.happiness > MAX_STAT)
    pet.happiness = MAX_STAT;

  if (pet.energy < MIN_STAT) {
    pet.energy = MIN_STAT;
    pet.is_sleeping = true; // Pass out
  }
  if (pet.hunger < MIN_STAT)
    pet.hunger = MIN_STAT;
  if (pet.happiness < MIN_STAT)
    pet.happiness = MIN_STAT;

  // Refresh UI
  update_sprite();
  update_bars();
  update_status_text();
}

static void perform_action(void) {
  switch (selected_action) {
  case ACTION_FEED:
    if (pet.is_sleeping)
      pet.is_sleeping = false; // Wake up

    // Start eating sequence. Actual hunger gain happens in game_tick
    if (pet.hunger < MAX_STAT) {
      pet.is_eating = true;
      play_beep_eat();
    }
    break;
  case ACTION_PLAY:
    if (pet.is_sleeping)
      pet.is_sleeping = false; // Wake up

    if (pet.energy > PLAY_COST) {
      pet.happiness += PLAY_RECOVERY;
      pet.energy -= PLAY_COST;
      play_beep_move();
    }
    break;
  case ACTION_SLEEP:
    pet.is_sleeping = !pet.is_sleeping; // Toggle sleep
    break;
  }
  game_tick(); // Immediate update
}

// Helper to create E-Ink styled bars with labels
static lv_obj_t *create_stat_bar(const char *label_text, int y_pos) {
  // Label
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, y_pos);

  // Bar
  lv_obj_t *bar = lv_bar_create(lv_scr_act());
  lv_obj_set_size(bar, 100, 12);
  lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 5, y_pos + 18);

  // Style: White Background with Black Border
  lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_border_color(bar, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN);

  // Style: Black Indicator
  lv_obj_set_style_bg_color(bar, lv_color_black(), LV_PART_INDICATOR);

  lv_bar_set_range(bar, 0, 100);
  return bar;
}

static void beaglegotchi_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  // --- Status Bar (Top Right) ---
  lbl_status = lv_label_create(lv_scr_act());
  lv_label_set_text(lbl_status, "Ready!");
  lv_obj_set_style_text_color(lbl_status, lv_color_black(), 0);
  // Fix for ghosting: Set opaque white background AND fixed width to cover all
  // potential text
  lv_obj_set_width(lbl_status, 200);
  lv_label_set_long_mode(lbl_status, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_bg_color(lbl_status, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(lbl_status, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(lbl_status, 2, 0);
  lv_obj_align(lbl_status, LV_ALIGN_TOP_RIGHT, -5, 5);

  // --- Stats Bars (Left Side) ---
  // Spaced out to avoid crowding
  bar_hunger = create_stat_bar("Hunger", 5);
  bar_energy = create_stat_bar("Energy", 45);
  bar_happy = create_stat_bar("Happy", 85);

  // --- Sprite (Center, slightly pushed down to clear bars) ---
  img_sprite = lv_img_create(lv_scr_act());
  lv_img_set_src(img_sprite, &standard);
  lv_obj_align(img_sprite, LV_ALIGN_CENTER, 20,
               10); // Shift right/down slightly

  // --- Action Menu (Bottom) ---
  menu_container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(menu_container, 350, 40);
  lv_obj_align(menu_container, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_set_flex_flow(menu_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(menu_container, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_width(menu_container, 0, 0); // Invisible container
  lv_obj_set_style_bg_opa(menu_container, LV_OPA_TRANSP, 0);

  for (int i = 0; i < NUM_ACTIONS; i++) {
    menu_labels[i] = lv_label_create(menu_container);
    lv_label_set_text(menu_labels[i], action_names[i]);
    lv_obj_set_style_pad_all(menu_labels[i], 15, 0);
    lv_obj_set_style_text_font(menu_labels[i], &lv_font_montserrat_14, 0);
  }

  // --- Tooltip (Very Bottom) ---
  lbl_tooltip = lv_label_create(lv_scr_act());
  lv_label_set_text(lbl_tooltip, "[< >] Move   [Select] Act");
  lv_obj_set_style_text_font(lbl_tooltip, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_tooltip, lv_color_black(), 0);
  lv_obj_align(lbl_tooltip, LV_ALIGN_BOTTOM_MID, 0, -2);

  // Initial Draw
  update_menu();
  update_sprite();
  update_bars();
  update_status_text();

  last_tick = k_uptime_get();
  last_input_time = k_uptime_get();
}

static void beaglegotchi_update(void) {
  int64_t now = k_uptime_get();

  // Input Handling (Debounced 200ms)
  if (now - last_input_time > 200) {
    if (gpio_pin_get_dt(&btn_left)) {
      selected_action--;
      if (selected_action < 0)
        selected_action = NUM_ACTIONS - 1;
      update_menu();
      play_beep_move();
      last_input_time = now;
    } else if (gpio_pin_get_dt(&btn_right)) {
      selected_action++;
      if (selected_action >= NUM_ACTIONS)
        selected_action = 0;
      update_menu();
      play_beep_move();
      last_input_time = now;
    } else if (gpio_pin_get_dt(&btn_select)) {
      perform_action();
      last_input_time = now;
    }
  }

  // Game Loop (1s Tick)
  if (now - last_tick > TICK_INTERVAL) {
    game_tick();
    last_tick = now;
  }
}

static void beaglegotchi_exit(void) {
  // State persists globally
}

App beaglegotchi_app = {.name = "Beaglegotchi",
                        .enter = beaglegotchi_enter,
                        .update = beaglegotchi_update,
                        .exit = beaglegotchi_exit};
