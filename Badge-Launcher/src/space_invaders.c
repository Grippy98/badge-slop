#include "space_invaders.h"
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(space_invaders, LOG_LEVEL_INF);

// --- Constants ---
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300
#define CELL_SIZE 20
#define COLS (SCREEN_WIDTH / CELL_SIZE)
#define ROWS (SCREEN_HEIGHT / CELL_SIZE)

#define PLAYER_Y (ROWS - 1)
#define MAX_PROJECTILES 5
#define MAX_INVADERS (COLS * (ROWS / 2))

// --- Game State ---
static int player_x; // Column Index
static int score;
static bool game_over;
static int64_t last_tick;
static int64_t last_move_tick;

typedef struct {
  int x; // Col
  int y; // Row
  bool active;
} Projectile;

typedef struct {
  int x; // Col
  int y; // Row
  bool active;
} Invader;

static Projectile projectiles[MAX_PROJECTILES];
static Invader invaders[MAX_INVADERS];
static int invader_count;

// Invader Movement State
static int invader_move_dir; // 1 (Right) or -1 (Left)
static int invader_move_timer;
static int invader_move_speed; // ms delay

// --- UI Objects ---
static lv_obj_t *main_cont;
static lv_obj_t *score_label;
static lv_obj_t *game_over_label;
static lv_obj_t *grid_cells[COLS][ROWS]; // Static Grid

// --- Input ---
static int btn_up_prev = 0;
static int btn_select_prev = 0;
// Note: Left/Right are polled directly

static void set_cell_color(int x, int y, lv_color_t color) {
  if (x >= 0 && x < COLS && y >= 0 && y < ROWS && grid_cells[x][y]) {
    lv_obj_set_style_bg_color(grid_cells[x][y], color, 0);
  }
}

static void render_game(void) {
  // 1. Clear Grid (White)
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      set_cell_color(x, y, lv_color_white());
    }
  }

  // 2. Player
  set_cell_color(player_x, PLAYER_Y, lv_color_black());

  // 3. Invaders
  for (int i = 0; i < MAX_INVADERS; i++) {
    if (invaders[i].active) {
      set_cell_color(invaders[i].x, invaders[i].y, lv_color_black());
    }
  }

  // 4. Projectiles (Small visual tweak? No, stick to grid for artifact safety)
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (projectiles[i].active) {
      set_cell_color(projectiles[i].x, projectiles[i].y, lv_color_black());
    }
  }
}

static void spawn_invaders(void) {
  invader_count = 0;
  // Spawn grid of invaders
  // Rows 1-5, Cols 2-17 (Leave margin)
  for (int r = 1; r <= 5; r++) {
    for (int c = 2; c < COLS - 2; c += 2) { // Spread out horizontally
      if (invader_count < MAX_INVADERS) {
        invaders[invader_count].x = c;
        invaders[invader_count].y = r;
        invaders[invader_count].active = true;
        invader_count++;
      }
    }
  }
}

static void reset_game(void) {
  player_x = COLS / 2;
  score = 0;
  game_over = false;

  // Reset Projectiles
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    projectiles[i].active = false;
  }

  spawn_invaders();
  invader_move_dir = 1;
  invader_move_speed = 800; // Start slow
  last_move_tick = k_uptime_get();

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  render_game();
}

static void space_invaders_enter(void) {
  main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(main_cont, lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

  // Create Static Grid
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      lv_obj_t *cell = lv_obj_create(main_cont);
      lv_obj_set_size(cell, CELL_SIZE, CELL_SIZE);
      lv_obj_set_pos(cell, x * CELL_SIZE, y * CELL_SIZE);
      lv_obj_set_style_border_width(cell, 0, 0);
      lv_obj_set_style_radius(cell, 0, 0);
      lv_obj_set_style_bg_color(cell, lv_color_white(), 0);
      grid_cells[x][y] = cell;
    }
  }

  // Score Label
  score_label = lv_label_create(main_cont);
  lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 5);
  lv_obj_set_style_text_color(score_label, lv_color_black(), 0);

  // Game Over Label
  game_over_label = lv_label_create(main_cont);
  lv_label_set_text(game_over_label, "GAME OVER\nPress UP to Restart");
  lv_obj_set_style_text_align(game_over_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(game_over_label);
  lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(game_over_label);

  reset_game();
  last_tick = k_uptime_get();
}

static void space_invaders_update(void) {
  int btn_up_curr = gpio_pin_get_dt(&btn_up);

  if (game_over) {
    if (btn_up_curr && !btn_up_prev) {
      reset_game();
    }
    btn_up_prev = btn_up_curr;
    return;
  }

  int64_t now = k_uptime_get();

  // Input Handling (Movement - Fast Polling)
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);

  // Move Player (Throttled but responsive)
  if ((now - last_tick > 100)) {
    if (btn_left_curr && player_x > 0)
      player_x--;
    if (btn_right_curr && player_x < COLS - 1)
      player_x++;
  }

  // Fire Projectile (Rising Edge of SELECT)
  int btn_select_curr = gpio_pin_get_dt(&btn_select);
  if (btn_select_curr && !btn_select_prev) {
    // Find empty projectile slot
    for (int i = 0; i < MAX_PROJECTILES; i++) {
      if (!projectiles[i].active) {
        projectiles[i].active = true;
        projectiles[i].x = player_x;
        projectiles[i].y = PLAYER_Y - 1;
        play_beep_move();
        break;
      }
    }
  }
  btn_select_prev = btn_select_curr;
  btn_up_prev = btn_up_curr;

  // --- Game Logic (Throttled) ---
  if (now - last_tick > 100) {

    // 1. Update Projectiles
    for (int i = 0; i < MAX_PROJECTILES; i++) {
      if (projectiles[i].active) {
        projectiles[i].y--; // Move Up
        if (projectiles[i].y < 0) {
          projectiles[i].active = false; // Off screen
        } else {
          // Collision Check vs Invaders
          for (int j = 0; j < MAX_INVADERS; j++) {
            if (invaders[j].active && invaders[j].x == projectiles[i].x &&
                invaders[j].y == projectiles[i].y) {
              invaders[j].active = false;
              projectiles[i].active = false;
              score += 10;
              play_beep_eat();
              lv_label_set_text_fmt(score_label, "Score: %d", score);
              break;
            }
          }
        }
      }
    }

    // 2. Move Invaders
    if (now - last_move_tick > invader_move_speed) {
      bool dir_changed = false;
      // Check edges
      for (int i = 0; i < MAX_INVADERS; i++) {
        if (invaders[i].active) {
          if (invader_move_dir == 1 && invaders[i].x >= COLS - 1) {
            dir_changed = true;
          } else if (invader_move_dir == -1 && invaders[i].x <= 0) {
            dir_changed = true;
          }
        }
      }

      if (dir_changed) {
        invader_move_dir = -invader_move_dir;
        // Move Down
        for (int i = 0; i < MAX_INVADERS; i++) {
          if (invaders[i].active)
            invaders[i].y++;
          // Game Over if reach bottom
          if (invaders[i].active && invaders[i].y >= PLAYER_Y) {
            game_over = true;
            lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
          }
        }
        // Increase speed
        if (invader_move_speed > 200)
          invader_move_speed -= 50;

      } else {
        // Determine if we should move ALL or just some?
        // Classic moves them one by one, but for Grid implementation, moving
        // all is safer for state. Move Horizontal
        for (int i = 0; i < MAX_INVADERS; i++) {
          if (invaders[i].active)
            invaders[i].x += invader_move_dir;
        }
      }
      last_move_tick = now;
    }

    render_game();
    last_tick = now;
  }
}

static void space_invaders_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App space_invaders_app = {.name = "Space Invaders",
                          .enter = space_invaders_enter,
                          .update = space_invaders_update,
                          .exit = space_invaders_exit};
