#include "beagle_man.h"
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(beagle_man, LOG_LEVEL_INF);

// --- Constants ---
#define COLS 20
#define ROWS 15
#define CELL_SIZE 20

// Map Tiles
#define TILE_EMPTY 0
#define TILE_WALL 1
#define TILE_DOT 2
#define TILE_POWER 3 // Not used yet, maybe later

// Entities
typedef struct {
  int x;
  int y;
  int dir_x;
  int dir_y;
  int next_dir_x; // Buffered Input
  int next_dir_y;
} Entity;

static Entity player;
static Entity ghost; // Single ghost for now
static int score;
static bool game_over;
static bool win_state;
static int64_t last_tick;
static int game_speed = 250;

// Map (1 = Wall, 0 = Empty/Dot placeholder)
// We will generate dots at runtime in empty spaces
static int map[COLS][ROWS];
static bool dots[COLS][ROWS];

// Static Map Layout (Simple symmetrical maze)
// 1: Wall, 0: Path
static const int initial_map[ROWS][COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // Row 1
    {1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1}, // Row 2
    {1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // Row 4
    {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1}, // Row 6
    {1, 1, 1, 1, 1, 0, 1, 1, 0, 0,
     0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, // Row 7 (Ghost House limit?)
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1}, // Row 8
    {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1}, // Row 10
    {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // Row 12
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

// --- UI Objects ---
static lv_obj_t *main_cont;
static lv_obj_t *score_label;
static lv_obj_t *game_over_label;
static lv_obj_t *grid_cells[COLS][ROWS];

static void set_cell_color(int x, int y, lv_color_t color) {
  if (x >= 0 && x < COLS && y >= 0 && y < ROWS && grid_cells[x][y]) {
    lv_obj_set_style_bg_color(grid_cells[x][y], color, 0);
  }
}

// Draw a small dot? We can just use a grey color or checkerboard?
// For pure grid: Wall=Black, Empty=White, Dot=Light Grey?
// Or: Wall=Black, Dot=Small square?
// Limitation: We are coloring the WHOLE cell.
// Solution: Dot = Light Grey Cell. Wall = Black. Player = Dark Blue/Black?
// Let's use:
// Wall: BLACK
// Empty: WHITE
// Dot: GREY (lv_palette_lighten(LV_PALETTE_GREY, 2))
// Player: RED (or Black if mono) -> Let's use Checkered or just Black/White.
// E-Ink is mono (mostly).
// Wall = Black.
// Dot = Light Grey (Dithering) -> might look messy.
// Let's allow Dot = White, Wall = Black.
// Player = Black (Blinking?) or just Black.
// Wait, if Player is Black and Wall is Black, how to distinguish?
// Player = Inverted?
// Let's make Walls GREY (Dithered Black) and Player/Ghost BLACK SOLID.
// Dots: White? (Invisible?)
// Actually, let's use: Wall = Black. Path = White. Dot = Small Black center?
// We can't draw sub-pixel in this static grid engine.
// Okay: Dot = Grey Cell. Player = Black. Wall = Black.
// Player vs Wall collisions prevent overlap, so visuals are fine.
// Dot vs Empty: Grey vs White.

static void render_game(void) {
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      lv_color_t color = lv_color_white();

      // 1. Static Map
      if (map[x][y] == TILE_WALL) {
        color = lv_color_black();
      } else if (dots[x][y]) {
        color = lv_palette_lighten(LV_PALETTE_GREY, 2); // Dot
      }

      // 2. Entities
      if (x == player.x && y == player.y) {
        color = lv_palette_main(LV_PALETTE_BLUE); // Player (Dark on mono)
      }
      if (x == ghost.x && y == ghost.y) {
        color = lv_palette_main(LV_PALETTE_RED); // Ghost
      }

      set_cell_color(x, y, color);
    }
  }
}

static void reset_game(void) {
  // Copy Map
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      map[x][y] = initial_map[y][x];
      dots[x][y] = (map[x][y] == 0); // Dot in every empty space
    }
  }

  // Player Start (Center-Left)
  player.x = 1;
  player.y = 1;
  player.dir_x = 0;
  player.dir_y = 0;
  player.next_dir_x = 1; // Start moving right
  player.next_dir_y = 0;

  // Ghost Start (Center-Right)
  ghost.x = COLS - 2;
  ghost.y = ROWS - 2;
  ghost.dir_x = 0;
  ghost.dir_y = 0;

  // Clear Start/End zones of dots
  dots[1][1] = false;
  dots[COLS - 2][ROWS - 2] = false;

  score = 0;
  game_over = false;
  win_state = false;

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  render_game();
}

static void beagle_man_enter(void) {
  main_cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(main_cont, lv_color_white(), 0);
  lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);

  // Grid
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      lv_obj_t *cell = lv_obj_create(main_cont);
      lv_obj_set_size(cell, CELL_SIZE, CELL_SIZE);
      lv_obj_set_pos(cell, x * CELL_SIZE, y * CELL_SIZE);
      lv_obj_set_style_border_width(cell, 0, 0);
      lv_obj_set_style_radius(cell, 0, 0);
      grid_cells[x][y] = cell;
    }
  }

  // UI
  score_label = lv_label_create(main_cont);
  lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 5);
  lv_obj_set_style_text_color(score_label, lv_color_white(),
                              0); // White text if on black wall?
  // Wait, top row is wall. Text might be hidden.
  // Let's modify map row 0 to be empty? No, walls needed.
  // Let's make label text INVERTED? Or just use a background?
  lv_obj_set_style_bg_color(score_label, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(score_label, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(score_label, lv_color_black(), 0);

  game_over_label = lv_label_create(main_cont);
  lv_label_set_text(game_over_label, "GAME OVER");
  lv_obj_center(game_over_label);
  lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(game_over_label, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(game_over_label, LV_OPA_COVER, 0);

  reset_game();
  last_tick = k_uptime_get();
}

static void try_move_entity(Entity *e) {
  // Try Next Dir
  if (e->next_dir_x != 0 || e->next_dir_y != 0) {
    int nx = e->x + e->next_dir_x;
    int ny = e->y + e->next_dir_y;
    if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS &&
        map[nx][ny] != TILE_WALL) {
      e->dir_x = e->next_dir_x;
      e->dir_y = e->next_dir_y;
    }
  }

  // Move in Current Dir
  int nx = e->x + e->dir_x;
  int ny = e->y + e->dir_y;

  if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS &&
      map[nx][ny] != TILE_WALL) {
    e->x = nx;
    e->y = ny;
  }
}

static void move_ghost_ai(void) {
  // Simple Stupid AI: Move towards player + random noise
  // 60% chance to chase, 40% random
  int tx, ty;

  if (sys_rand32_get() % 100 < 60) {
    tx = player.x;
    ty = player.y;
  } else {
    tx = sys_rand32_get() % COLS;
    ty = sys_rand32_get() % ROWS;
  }

  // Directions: Up, Down, Left, Right
  int dx[] = {0, 0, -1, 1};
  int dy[] = {-1, 1, 0, 0};

  int best_dist = 9999;
  int best_dir = -1;

  // Filter out reverse direction to prevent jitter?
  // Nah, keep it simple.

  for (int i = 0; i < 4; i++) {
    int nx = ghost.x + dx[i];
    int ny = ghost.y + dy[i];

    if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS &&
        map[nx][ny] != TILE_WALL) {
      int dist = (nx - tx) * (nx - tx) + (ny - ty) * (ny - ty);
      if (dist < best_dist) {
        best_dist = dist;
        best_dir = i;
      }
    }
  }

  if (best_dir != -1) {
    ghost.x += dx[best_dir];
    ghost.y += dy[best_dir];
  }
}

static void beagle_man_update(void) {
  int btn_up_curr = gpio_pin_get_dt(&btn_up);

  // DOWN is BROKEN on hardware -> Use SELECT as DOWN
  int btn_select_curr = gpio_pin_get_dt(&btn_select);

  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);

  // Input Buffering
  if (btn_up_curr) {
    player.next_dir_x = 0;
    player.next_dir_y = -1;
  }
  if (btn_select_curr) {
    player.next_dir_x = 0;
    player.next_dir_y = 1;
  } // Mapped Down
  if (btn_left_curr) {
    player.next_dir_x = -1;
    player.next_dir_y = 0;
  }
  if (btn_right_curr) {
    player.next_dir_x = 1;
    player.next_dir_y = 0;
  }

  static int btn_up_prev_gm = 0;
  if (game_over) {
    if (btn_up_curr && !btn_up_prev_gm) {
      reset_game();
    }
    btn_up_prev_gm = btn_up_curr;
    return;
  }
  btn_up_prev_gm = btn_up_curr;

  int64_t now = k_uptime_get();
  if (now - last_tick > game_speed) {

    // 1. Move Player
    try_move_entity(&player);

    // 2. Eat Dot
    if (dots[player.x][player.y]) {
      dots[player.x][player.y] = false;
      score += 10;
      play_beep_eat();
      lv_label_set_text_fmt(score_label, "Score: %d", score);
      // Speed up?
      if (score % 100 == 0 && game_speed > 150)
        game_speed -= 10;
    }

    // 3. Move Ghost (Slower? No, same speed for tension)
    // Move ghost every other tick? No, same speed.
    move_ghost_ai();

    // 4. Collision
    if (player.x == ghost.x && player.y == ghost.y) {
      game_over = true;
      play_beep_die();
      lv_label_set_text(game_over_label, "GAME OVER\nUP to Restart");
      lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
    }

    render_game();
    last_tick = now;
  }
}

static void beagle_man_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App beagle_man_app = {.name = "BeagleMan",
                      .enter = beagle_man_enter,
                      .update = beagle_man_update,
                      .exit = beagle_man_exit};
