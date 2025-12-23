#include "snake_game.h"
#include <zephyr/random/random.h>

LOG_MODULE_DECLARE(badge_launcher);

/* Game Constants */
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 300
#define BLOCK_SIZE 20
#define GRID_COLS (SCREEN_WIDTH / BLOCK_SIZE)
#define GRID_ROWS (SCREEN_HEIGHT / BLOCK_SIZE)
#define MAX_SNAKE_LEN (GRID_COLS * GRID_ROWS)

/* Types */
typedef struct {
  int x;
  int y;
} Point;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE } Direction;

/* State */
static Point snake[MAX_SNAKE_LEN];
static int snake_len;
static Direction current_dir;
static Direction next_dir;
static Point food;
static bool game_over;
static bool paused;
static int score;
static int64_t last_tick;

/* UI Elements */
static lv_obj_t *score_label;
static lv_obj_t *game_over_label;
static lv_obj_t *paused_label;
static lv_obj_t *snake_body_container;

/* Static Grid of Objects */
static lv_obj_t *grid_objs[GRID_COLS][GRID_ROWS];

static void set_block_color(int x, int y, lv_color_t color) {
  if (x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS && grid_objs[x][y]) {
    lv_obj_set_style_bg_color(grid_objs[x][y], color, 0);
  }
}

static void spawn_food() {
  bool on_snake;
  do {
    on_snake = false;
    food.x = sys_rand32_get() % GRID_COLS;
    food.y = sys_rand32_get() % GRID_ROWS;

    for (int i = 0; i < snake_len; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        on_snake = true;
        break;
      }
    }
  } while (on_snake);

  set_block_color(food.x, food.y, lv_color_black());
}

static void init_game_logic() {
  // Clear Grid visually
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      set_block_color(x, y, lv_color_white());
    }
  }

  snake_len = 3;
  snake[0].x = GRID_COLS / 2;
  snake[0].y = GRID_ROWS / 2;
  snake[1].x = snake[0].x;
  snake[1].y = snake[0].y + 1;
  snake[2].x = snake[0].x;
  snake[2].y = snake[0].y + 2;

  // Draw initial snake
  for (int i = 0; i < snake_len; i++) {
    set_block_color(snake[i].x, snake[i].y, lv_color_black());
  }

  current_dir = DIR_UP;
  next_dir = DIR_UP;
  game_over = false;
  paused = false;
  score = 0;

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (paused_label)
    lv_obj_add_flag(paused_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  spawn_food();
}

/* Input Queue */
#define QUEUE_SIZE 2
static Direction move_queue[QUEUE_SIZE];
static int queue_count = 0;
static int queue_head = 0;
static int queue_tail = 0;

static int prev_btn_up = 0;
static int prev_btn_down = 0;
static int prev_btn_left = 0;
static int prev_btn_right = 0;
static int prev_btn_select = 0;

static void queue_reset() {
  queue_count = 0;
  queue_head = 0;
  queue_tail = 0;
}

static Direction get_last_planned_dir() {
  if (queue_count == 0) {
    return current_dir;
  }
  // Tail is where we add, so the last added item is at (tail - 1)
  int last_idx = (queue_tail - 1 + QUEUE_SIZE) % QUEUE_SIZE;
  return move_queue[last_idx];
}

static bool is_opposite(Direction d1, Direction d2) {
  if (d1 == DIR_UP && d2 == DIR_DOWN)
    return true;
  if (d1 == DIR_DOWN && d2 == DIR_UP)
    return true;
  if (d1 == DIR_LEFT && d2 == DIR_RIGHT)
    return true;
  if (d1 == DIR_RIGHT && d2 == DIR_LEFT)
    return true;
  return false;
}

static void try_enqueue_move(Direction dir) {
  if (queue_count >= QUEUE_SIZE) {
    return; // Full
  }

  Direction last_dir = get_last_planned_dir();

  // Don't allow 180 turns
  if (is_opposite(dir, last_dir)) {
    return;
  }

  // Don't duplicate moves (optional, but good)
  if (dir == last_dir) {
    return;
  }

  move_queue[queue_tail] = dir;
  queue_tail = (queue_tail + 1) % QUEUE_SIZE;
  queue_count++;
}

static void update_logic() {
  if (game_over || paused)
    return;

  play_beep_move();

  // Dequeue Next Move
  if (queue_count > 0) {
    next_dir = move_queue[queue_head];
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;
  }

  current_dir = next_dir;
  Point new_head = snake[0];

  switch (current_dir) {
  case DIR_UP:
    new_head.y--;
    break;
  case DIR_DOWN:
    new_head.y++;
    break;
  case DIR_LEFT:
    new_head.x--;
    break;
  case DIR_RIGHT:
    new_head.x++;
    break;
  default:
    break;
  }

  // Wrap
  if (new_head.x < 0)
    new_head.x = GRID_COLS - 1;
  if (new_head.x >= GRID_COLS)
    new_head.x = 0;
  if (new_head.y < 0)
    new_head.y = GRID_ROWS - 1;
  if (new_head.y >= GRID_ROWS)
    new_head.y = 0;

  // Self Collision
  for (int i = 0; i < snake_len - 1; i++) {
    if (new_head.x == snake[i].x && new_head.y == snake[i].y) {
      game_over = true;
      lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
      play_beep_die();
      return;
    }
  }

  // Check Food
  bool ate = (new_head.x == food.x && new_head.y == food.y);

  if (ate) {
    if (snake_len < MAX_SNAKE_LEN) {
      snake_len++;
    }
    score += 10;
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    play_beep_eat();
    spawn_food();
  }

  // Shift Data
  for (int i = snake_len - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = new_head;

  // Redraw Board
  // 1. Clear All
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      set_block_color(x, y, lv_color_white());
    }
  }
  // 2. Draw Snake
  for (int i = 0; i < snake_len; i++) {
    set_block_color(snake[i].x, snake[i].y, lv_color_black());
  }
  // 3. Draw Food
  set_block_color(food.x, food.y, lv_color_black());
}

static void snake_enter(void) {
  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

  snake_body_container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(snake_body_container, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_opa(snake_body_container, 0, 0);
  lv_obj_set_style_border_width(snake_body_container, 0, 0);
  lv_obj_align(snake_body_container, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_pad_all(snake_body_container, 0, 0);
  lv_obj_clear_flag(snake_body_container, LV_OBJ_FLAG_SCROLLABLE);

  // Initialize Static Grid
  for (int x = 0; x < GRID_COLS; x++) {
    for (int y = 0; y < GRID_ROWS; y++) {
      lv_obj_t *obj = lv_obj_create(snake_body_container);
      if (!obj) {
        LOG_ERR("Failed to create grid object at %d,%d", x, y);
        continue;
      }
      lv_obj_set_size(obj, BLOCK_SIZE, BLOCK_SIZE);
      lv_obj_set_pos(obj, x * BLOCK_SIZE, y * BLOCK_SIZE);
      lv_obj_set_style_radius(obj, 0, 0);
      lv_obj_set_style_border_width(obj, 0, 0);
      lv_obj_set_style_bg_color(obj, lv_color_white(), 0); // Start White
      grid_objs[x][y] = obj;
    }
  }

  score_label = lv_label_create(lv_scr_act());
  lv_label_set_text(score_label, "Score: 0");
  lv_obj_set_style_text_color(score_label, lv_color_black(), 0);
  lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 5, 5);
  // Ensure label is above grid
  lv_obj_move_foreground(score_label);

  game_over_label = lv_label_create(lv_scr_act());
  lv_label_set_text(game_over_label, "GAME OVER");
  lv_obj_set_style_text_color(game_over_label, lv_color_make(0, 0, 0), 0);
  lv_obj_align(game_over_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(game_over_label);

  paused_label = lv_label_create(lv_scr_act());
  lv_label_set_text(paused_label, "PAUSED");
  lv_obj_set_style_text_color(paused_label, lv_color_black(), 0);
  lv_obj_align(paused_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(paused_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(paused_label);

  init_game_logic();
  queue_reset();
  last_tick = k_uptime_get();
}

static void snake_update(void) {
  // Input Polling (Edge Detection)
  int curr_up = gpio_pin_get_dt(&btn_up);
  int curr_down = gpio_pin_get_dt(&btn_down);
  int curr_left = gpio_pin_get_dt(&btn_left);
  int curr_right = gpio_pin_get_dt(&btn_right);
  int curr_select = gpio_pin_get_dt(&btn_select);

  if (!paused) {
    if (curr_up && !prev_btn_up)
      try_enqueue_move(DIR_UP);
    if (curr_down && !prev_btn_down)
      try_enqueue_move(DIR_DOWN);
    if (curr_left && !prev_btn_left)
      try_enqueue_move(DIR_LEFT);
    if (curr_right && !prev_btn_right)
      try_enqueue_move(DIR_RIGHT);
  }

  if (curr_select && !prev_btn_select) {
    if (game_over) {
      init_game_logic();
      queue_reset();
    } else {
      paused = !paused;
      if (paused) {
        lv_obj_clear_flag(paused_label, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(paused_label, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }

  prev_btn_up = curr_up;
  prev_btn_down = curr_down;
  prev_btn_left = curr_left;
  prev_btn_right = curr_right;
  prev_btn_select = curr_select;

  int64_t now = k_uptime_get();
  if (now - last_tick > 100) {
    update_logic();
    last_tick = now;
  }
}

static void snake_exit(void) {
  // Cleanup happens via screen clean in main
}

App snake_game_app = {.name = "Snake",
                      .enter = snake_enter,
                      .update = snake_update,
                      .exit = snake_exit};
