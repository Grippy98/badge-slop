#include "brick_breaker.h"
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(brick_breaker, LOG_LEVEL_INF);

// --- Constants ---
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300
#define CELL_SIZE 20
#define COLS (SCREEN_WIDTH / CELL_SIZE)
#define ROWS (SCREEN_HEIGHT / CELL_SIZE)

#define PADDLE_Y (ROWS - 1)
#define MAX_BRICKS (COLS * 5) // Top 5 rows

// --- Game State ---
static int paddle_x;         // Center col of paddle
static int paddle_width = 3; // 3 cells wide
static int ball_x;
static int ball_y;
static int ball_dx;
static int ball_dy;
static int score;
static bool game_over;
static int64_t last_tick;

typedef struct {
  int x;
  int y;
  bool active;
} Brick;

static Brick bricks[MAX_BRICKS];

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

static void render_game(void) {
  // 1. Clear Grid (White)
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      set_cell_color(x, y, lv_color_white());
    }
  }

  // 2. Paddle (Centered at paddle_x, width is odd)
  int offset = paddle_width / 2;
  for (int i = -offset; i <= offset; i++) {
    set_cell_color(paddle_x + i, PADDLE_Y, lv_color_black());
  }

  // 3. Bricks
  for (int i = 0; i < MAX_BRICKS; i++) {
    if (bricks[i].active) {
      set_cell_color(bricks[i].x, bricks[i].y, lv_color_black());
    }
  }

  // 4. Ball
  set_cell_color(ball_x, ball_y, lv_color_black());
}

static void reset_game(void) {
  paddle_x = COLS / 2;
  ball_x = paddle_x;
  ball_y = PADDLE_Y - 1;
  ball_dx = (sys_rand32_get() % 2 == 0) ? 1 : -1;
  ball_dy = -1; // Start moving up
  score = 0;
  game_over = false;

  // Reset Bricks (Rows 1-5)
  int idx = 0;
  for (int y = 1; y <= 5; y++) {
    for (int x = 1; x < COLS - 1; x++) {
      if (idx < MAX_BRICKS) {
        bricks[idx].x = x;
        bricks[idx].y = y;
        bricks[idx].active = true;
        idx++;
      }
    }
  }
  // Deactivate remaining
  for (; idx < MAX_BRICKS; idx++)
    bricks[idx].active = false;

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  render_game();
}

static void brick_breaker_enter(void) {
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

static void brick_breaker_update(void) {
  int btn_up_curr = gpio_pin_get_dt(&btn_up);

  if (game_over) {
    if (btn_up_curr && !btn_up_prev) {
      reset_game();
    }
    btn_up_prev = btn_up_curr;
    return;
  }

  int64_t now = k_uptime_get();

  // --- Input (Paddle Move) ---
  // Fast polling for responsiveness
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);

  int offset = paddle_width / 2;

  if (now - last_tick > 80) { // Throttle input slightly but keep it fast
    if (btn_left_curr && paddle_x > offset)
      paddle_x--;
    if (btn_right_curr && paddle_x < COLS - 1 - offset)
      paddle_x++;
  }

  // --- Physics (Ball Logic) ---
  // Update ball every 150ms (Medium speed)
  static int64_t last_ball_tick = 0;
  if (now - last_ball_tick > 150) {
    last_ball_tick = now;

    int next_x = ball_x + ball_dx;
    int next_y = ball_y + ball_dy;

    // Wall Collision (X)
    if (next_x < 0 || next_x >= COLS) {
      ball_dx = -ball_dx;
      next_x = ball_x + ball_dx; // Bounce back immediately
    }

    // Wall Collision (Top Y)
    if (next_y < 0) {
      ball_dy = -ball_dy;
      next_y = ball_y + ball_dy;
    }

    // Paddle Collision
    if (next_y >= PADDLE_Y) { // Reached bottom row
      // Check if within paddle range
      if (next_x >= paddle_x - offset && next_x <= paddle_x + offset) {
        ball_dy = -ball_dy; // Bounce Up
        next_y = ball_y + ball_dy;
        play_beep_move();

        // Angle change? (Simple physics for grid: randomized reflection?)
        // Maybe if hitting edges of paddle, angle changes?
        // Grid is too coarse for complex angles, just keep simple 45 deg.
      } else {
        // Missed -> Game Over
        game_over = true;
        lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
      }
    }

    // Brick Collision
    for (int i = 0; i < MAX_BRICKS; i++) {
      if (bricks[i].active && bricks[i].x == next_x && bricks[i].y == next_y) {
        bricks[i].active = false;
        ball_dy = -ball_dy; // Bounce vertically (simple assumption)
        next_y = ball_y + ball_dy;
        score += 10;
        play_beep_eat();
        lv_label_set_text_fmt(score_label, "Score: %d", score);
        break; // Only hit one brick per frame
      }
    }

    ball_x = next_x;
    ball_y = next_y;

    render_game();
  }

  last_tick = now;
  btn_up_prev = btn_up_curr;
}

static void brick_breaker_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App brick_breaker_app = {.name = "Brick Breaker",
                         .enter = brick_breaker_enter,
                         .update = brick_breaker_update,
                         .exit = brick_breaker_exit};
