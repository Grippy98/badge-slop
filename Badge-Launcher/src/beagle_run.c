#include "beagle_run.h"
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(beagle_run, LOG_LEVEL_INF);

// --- Constants ---
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300
#define CELL_SIZE 20
#define COLS (SCREEN_WIDTH / CELL_SIZE)
#define ROWS (SCREEN_HEIGHT / CELL_SIZE)

#define GROUND_Y (SCREEN_HEIGHT - CELL_SIZE) // Ground is bottom row
#define PLAYER_X 40                          // Fixed X pixel position (Col 2)

#define GRAVITY 4
#define JUMP_FORCE -35  // Higher jump for larger screen
#define SCROLL_SPEED 20 // 1 Cell per frame

// --- Game State ---
static int player_y;
static int player_vy;
static bool is_jumping;
static int score;
static bool game_over;
static int64_t last_tick;

typedef struct {
  int x;
  bool active;
} Obstacle;

#define MAX_OBSTACLES 5
static Obstacle obstacles[MAX_OBSTACLES];
static int next_spawn_distance;

// --- UI Objects ---
static lv_obj_t *main_cont;
static lv_obj_t *score_label;
static lv_obj_t *game_over_label;
static lv_obj_t *grid_cells[COLS][ROWS]; // Static Grid

// --- Input ---
static int btn_up_prev = 0;

static void set_cell_color(int x, int y, lv_color_t color) {
  if (x >= 0 && x < COLS && y >= 0 && y < ROWS && grid_cells[x][y]) {
    lv_obj_set_style_bg_color(grid_cells[x][y], color, 0);
  }
}

static void draw_rect_to_grid(int px, int py, int w, int h, lv_color_t color) {
  // Convert Pixel Rect to Grid Cells
  int start_col = px / CELL_SIZE;
  int end_col = (px + w - 1) / CELL_SIZE;
  int start_row = py / CELL_SIZE;
  int end_row = (py + h - 1) / CELL_SIZE;

  for (int c = start_col; c <= end_col; c++) {
    for (int r = start_row; r <= end_row; r++) {
      set_cell_color(c, r, color);
    }
  }
}

static void render_game(void) {
  // 1. Clear Grid (White)
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      set_cell_color(x, y, lv_color_white());
    }
  }

  // 2. Draw Ground (Static Line handled in enter, not per-frame)
  // draw_rect_to_grid(0, GROUND_Y, SCREEN_WIDTH, CELL_SIZE, lv_color_black());

  // 3. Draw Player
  // Player is CELL_SIZE x CELL_SIZE
  draw_rect_to_grid(PLAYER_X, player_y - CELL_SIZE, CELL_SIZE, CELL_SIZE,
                    lv_color_black());

  // 4. Draw Obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      draw_rect_to_grid(obstacles[i].x, GROUND_Y - CELL_SIZE, CELL_SIZE,
                        CELL_SIZE, lv_color_black());
    }
  }
}

static void reset_game(void) {
  player_y = GROUND_Y;
  player_vy = 0;
  is_jumping = false;
  score = 0;
  game_over = false;
  next_spawn_distance = 0;

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
    obstacles[i].x = SCREEN_WIDTH + 100;
  }

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  render_game();
}

static void beagle_run_enter(void) {
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
      lv_obj_set_style_border_width(cell, 0, 0); // No Borders for seamless look
      lv_obj_set_style_radius(cell, 0, 0);
      lv_obj_set_style_bg_color(cell, lv_color_white(), 0);
      grid_cells[x][y] = cell;
    }
  }

  // Static Ground Line (Thinner, independent of grid)
  lv_obj_t *ground_line = lv_obj_create(main_cont);
  lv_obj_set_size(ground_line, SCREEN_WIDTH, 4); // 4px thin line
  lv_obj_set_pos(ground_line, 0, GROUND_Y);
  lv_obj_set_style_bg_color(ground_line, lv_color_black(), 0);
  lv_obj_set_style_border_width(ground_line, 0, 0);
  lv_obj_move_foreground(ground_line); // On top of grid cells

  // Score Label (Overlay)
  score_label = lv_label_create(main_cont);
  lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_set_style_text_color(score_label, lv_color_black(), 0);

  // Game Over Label
  game_over_label = lv_label_create(main_cont);
  lv_label_set_text(game_over_label, "GAME OVER\nPress UP to Restart");
  lv_obj_set_style_text_align(game_over_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(game_over_label);
  lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(game_over_label); // Ensure on top of grid

  reset_game();
  last_tick = k_uptime_get();
}

static void beagle_run_update(void) {
  int btn_up_curr = gpio_pin_get_dt(&btn_up);

  if (game_over) {
    if (btn_up_curr && !btn_up_prev) {
      reset_game();
    }
    btn_up_prev = btn_up_curr;
    return;
  }

  // Loop Interval: 50ms = 20 FPS (Good for discrete stepping if speed is high)
  // If we want 1-cell moves:
  // With Speed 20px, 50ms -> 400px/s -> 1 screen/s.
  int64_t now = k_uptime_get();
  if (now - last_tick < 50)
    return;
  last_tick = now;

  // --- Input (Jump) ---
  if (btn_up_curr && !btn_up_prev) {
    if (!is_jumping) {
      if (player_y >= GROUND_Y) {
        player_vy = JUMP_FORCE;
        is_jumping = true;
      }
    }
  }
  btn_up_prev = btn_up_curr;

  // --- Physics ---
  player_y += player_vy;
  player_vy += GRAVITY;

  if (player_y > GROUND_Y) {
    player_y = GROUND_Y;
    player_vy = 0;
    is_jumping = false;
  }

  // --- Obstacles ---
  next_spawn_distance -= SCROLL_SPEED;
  if (next_spawn_distance <= 0) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (!obstacles[i].active) {
        obstacles[i].active = true;
        obstacles[i].x = SCREEN_WIDTH + CELL_SIZE; // Start offscreen
        // Random spawn 200-600px
        next_spawn_distance = 200 + (sys_rand32_get() % 400);
        break;
      }
    }
  }

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].x -= SCROLL_SPEED;

      // Collision AABB
      int p_left = PLAYER_X;
      int p_right = p_left + CELL_SIZE;
      int p_top = player_y - CELL_SIZE;
      int p_bottom = player_y;

      int o_left = obstacles[i].x;
      int o_right = o_left + CELL_SIZE;
      int o_top = GROUND_Y - CELL_SIZE;
      int o_bottom = GROUND_Y;

      // Simple logic: if ranges overlap
      if (p_right > o_left && p_left < o_right && p_bottom > o_top &&
          p_top < o_bottom) {
        game_over = true;
        lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(game_over_label);
      }

      if (obstacles[i].x < -CELL_SIZE) {
        obstacles[i].active = false;
        score++;
        lv_label_set_text_fmt(score_label, "Score: %d", score);
        lv_obj_move_foreground(score_label);
      }
    }
  }

  render_game();
}

static void beagle_run_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App beagle_run_app = {.name = "Beagle Run",
                      .enter = beagle_run_enter,
                      .update = beagle_run_update,
                      .exit = beagle_run_exit};
