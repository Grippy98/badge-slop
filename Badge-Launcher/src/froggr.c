#include "froggr.h"
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(froggr, LOG_LEVEL_INF);

// --- Constants ---
#define COLS 20
#define ROWS 15
#define CELL_SIZE 20

// Lane Types
#define LANE_SAFE 0
#define LANE_ROAD 1
#define LANE_RIVER 2
#define LANE_GOAL 3

// Entity (Obstacle/Log)
typedef struct {
  int x;
  int width;
  int type; // 0=Car, 1=Log
} Obstacle;

// Lane Config
typedef struct {
  int type;
  int speed;     // Tick interval (lower = faster)
  int direction; // 1 = Right, -1 = Left
  int64_t last_move;
  Obstacle obstacles[5]; // Max obstacles per lane
  int obstacle_count;
} Lane;

static Lane lanes[ROWS];
static int player_x;
static int player_y;
static int score;
static bool game_over;
static bool win_state;

// UI
static lv_obj_t *main_cont;
static lv_obj_t *score_label;
static lv_obj_t *game_over_label;
static lv_obj_t *grid_cells[COLS][ROWS];

static void init_lanes(void) {
  // Top (Goal)
  lanes[0].type = LANE_GOAL;

  // River (Rows 1-5)
  for (int y = 1; y <= 5; y++) {
    lanes[y].type = LANE_RIVER;
    lanes[y].direction = (y % 2 == 0) ? 1 : -1;
    lanes[y].speed = 150 + (sys_rand32_get() % 100);
    lanes[y].obstacle_count = 2 + (sys_rand32_get() % 2); // 2-3 logs
    for (int i = 0; i < lanes[y].obstacle_count; i++) {
      lanes[y].obstacles[i].x = (i * 6) + (sys_rand32_get() % 3);
      lanes[y].obstacles[i].width = 3; // Log width
      lanes[y].obstacles[i].type = 1;  // Log
    }
  }

  // Safe Strip (Row 6)
  lanes[6].type = LANE_SAFE;

  // Road (Rows 7-11)
  for (int y = 7; y <= 11; y++) {
    lanes[y].type = LANE_ROAD;
    lanes[y].direction = (y % 2 != 0) ? 1 : -1;
    lanes[y].speed = 100 + (sys_rand32_get() % 150);
    lanes[y].obstacle_count = 2 + (sys_rand32_get() % 2);
    for (int i = 0; i < lanes[y].obstacle_count; i++) {
      lanes[y].obstacles[i].x = (i * 7) + (sys_rand32_get() % 4);
      lanes[y].obstacles[i].width = 1; // Car width
      lanes[y].obstacles[i].type = 0;  // Car
    }
  }

  // Safe Start (Rows 12-14)
  for (int y = 12; y < ROWS; y++) {
    lanes[y].type = LANE_SAFE;
  }
}

static void set_cell_color(int x, int y, lv_color_t color) {
  if (x >= 0 && x < COLS && y >= 0 && y < ROWS && grid_cells[x][y]) {
    lv_obj_set_style_bg_color(grid_cells[x][y], color, 0);
  }
}

static void render_game(void) {
  for (int y = 0; y < ROWS; y++) {
    Lane *l = &lanes[y];

    // Base Lane Color
    lv_color_t bg_color = lv_color_white();
    if (l->type == LANE_ROAD)
      bg_color = lv_color_white(); // White Road
    else if (l->type == LANE_RIVER)
      bg_color = lv_color_black(); // Black River (Water)
    else if (l->type == LANE_GOAL)
      bg_color = lv_palette_lighten(LV_PALETTE_GREY, 2);

    // Draw Lane Background
    for (int x = 0; x < COLS; x++) {
      set_cell_color(x, y, bg_color);
    }

    // Draw Obstacles
    if (l->type == LANE_ROAD || l->type == LANE_RIVER) {
      for (int i = 0; i < l->obstacle_count; i++) {
        Obstacle *o = &l->obstacles[i];
        lv_color_t obj_color =
            (o->type == 1) ? lv_color_white() : lv_color_black();

        // If Log on River -> White Log on Black Water
        // If Car on Road -> Black Car on White Road

        for (int w = 0; w < o->width; w++) {
          int draw_x = o->x + w;
          // Wrap Rendering logic not strictly needed if we wrap coords, but
          // good for visual Actually we wrap logically in update, so x should
          // be 0-19. But if object wraps edge? Let's keep it simple: objects
          // wrap instantly.
          if (draw_x >= COLS)
            draw_x -= COLS;
          if (draw_x < 0)
            draw_x += COLS;

          set_cell_color(draw_x, y, obj_color);
        }
      }
    }
  }

  // Draw Player
  set_cell_color(player_x, player_y, lv_color_black()); // Black Frog
}

static void check_collision(void) {
  Lane *l = &lanes[player_y];

  if (l->type == LANE_ROAD) {
    // Hit by Car?
    for (int i = 0; i < l->obstacle_count; i++) {
      Obstacle *o = &l->obstacles[i];
      // Simple X-Range check (wrapping handled?)
      // Assuming no wrapping overlap for now
      if (player_x >= o->x && player_x < o->x + o->width) {
        game_over = true;
        play_beep_die();
        return;
      }
    }
  } else if (l->type == LANE_RIVER) {
    // Must be on Log
    bool on_log = false;
    for (int i = 0; i < l->obstacle_count; i++) {
      Obstacle *o = &l->obstacles[i];
      if (player_x >= o->x && player_x < o->x + o->width) {
        on_log = true;
        break;
      }
    }
    if (!on_log) {
      game_over = true; // Drowned
      play_beep_die();
    }
  } else if (l->type == LANE_GOAL) {
    lv_label_set_text(game_over_label, "WINNER!\nUP to Restart");
    lv_obj_clear_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
    win_state = true;
    game_over = true;
    score += 100;
    lv_label_set_text_fmt(score_label, "Score: %d", score);
    play_beep_eat();
  }
}

static void reset_game(void) {
  init_lanes();
  player_x = COLS / 2;
  player_y = ROWS - 1;
  score = 0;
  game_over = false;
  win_state = false;

  if (game_over_label)
    lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  if (score_label)
    lv_label_set_text(score_label, "Score: 0");

  render_game();
}

static void froggr_enter(void) {
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

  score_label = lv_label_create(main_cont);
  lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 5);
  lv_obj_set_style_bg_color(score_label, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(score_label, LV_OPA_COVER, 0);

  game_over_label = lv_label_create(main_cont);
  lv_label_set_text(game_over_label, "SPLAT!");
  lv_obj_center(game_over_label);
  lv_obj_add_flag(game_over_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(game_over_label, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(game_over_label, LV_OPA_COVER, 0);

  reset_game();
}

static void froggr_update(void) {
  int btn_up_curr = gpio_pin_get_dt(&btn_up);
  static int btn_up_prev_fr = 0;
  static int btn_right_prev = 0;
  static int btn_left_prev = 0;
  static int btn_down_prev = 0;

  if (game_over) {
    if (btn_up_curr && !btn_up_prev_fr) {
      reset_game();
    }
    btn_up_prev_fr = btn_up_curr;
    return;
  }

  // Input
  int btn_left_curr = gpio_pin_get_dt(&btn_left);
  int btn_right_curr = gpio_pin_get_dt(&btn_right);
  int btn_down_curr = gpio_pin_get_dt(&btn_down);

  bool moved = false;

  if (btn_up_curr && !btn_up_prev_fr) {
    if (player_y > 0) {
      player_y--;
      score += 10;
      moved = true;
    }
  }
  if (btn_down_curr && !btn_down_prev) {
    if (player_y < ROWS - 1) {
      player_y++;
      score -= 5;
      moved = true;
    }
  }
  if (btn_left_curr && !btn_left_prev) {
    if (player_x > 0) {
      player_x--;
      moved = true;
    }
  }
  if (btn_right_curr && !btn_right_prev) {
    if (player_x < COLS - 1) {
      player_x++;
      moved = true;
    }
  }

  btn_up_prev_fr = btn_up_curr;
  btn_left_prev = btn_left_curr;
  btn_right_prev = btn_right_curr;
  btn_down_prev = btn_down_curr;

  if (moved) {
    play_beep_move();
    lv_label_set_text_fmt(score_label, "Score: %d", score);
  }

  // Update Lanes
  int64_t now = k_uptime_get();
  bool need_render = moved;

  for (int y = 0; y < ROWS; y++) {
    Lane *l = &lanes[y];
    if (l->type == LANE_ROAD || l->type == LANE_RIVER) {
      if (now - l->last_move > l->speed) {
        // Move Obstacles
        for (int i = 0; i < l->obstacle_count; i++) {
          l->obstacles[i].x += l->direction;
          // Wrap
          if (l->obstacles[i].x >= COLS)
            l->obstacles[i].x = -l->obstacles[i].width;
          if (l->obstacles[i].x < -l->obstacles[i].width)
            l->obstacles[i].x = COLS;
        }

        // Move Player if on Log (River)
        if (l->type == LANE_RIVER && player_y == y) {
          player_x += l->direction;
          if (player_x < 0)
            player_x = 0; // Don't wrap player, clamp
          if (player_x >= COLS)
            player_x = COLS - 1;
        }

        l->last_move = now;
        need_render = true;
      }
    }
  }

  if (need_render) {
    check_collision(); // Check BEFORE rendering (e.g. if log moved us or car
                       // hit us)
    if (!game_over) {
      render_game();
    }
  }
}

static void froggr_exit(void) {
  if (main_cont) {
    lv_obj_del(main_cont);
    main_cont = NULL;
  }
}

App froggr_app = {.name = "Froggr",
                  .enter = froggr_enter,
                  .update = froggr_update,
                  .exit = froggr_exit};
