#include <gtp_traffic_game.h>
#include <gtp_display.h>
#include <gtp_buttons.h>
#include <gtp_game.h>
#include <gtp_sound.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_traffic_game, CONFIG_GTPTRAFFICGAME_LOG_LEVEL);

static const char menu_title_escape[] = "traffic escape game";
static const char menu_title_catch[] = "traffic catch game";

// the display buffer that holds data
static uint8_t buf[DISPLAY_WIDTH];
static uint8_t buf_obstacles[DISPLAY_WIDTH];
static bool game_is_finished = false;
static uint8_t player_vertical_pos = 0;

K_SEM_DEFINE(traffic_escape_game_start, 0, 1);
K_SEM_DEFINE(traffic_catch_game_start, 0, 1);

typedef enum {
	TRAFFIC_GAME_ESCAPE = 0,
	TRAFFIC_GAME_CATCH = 1
} traffic_game_mode_t;

static traffic_game_mode_t game_mode;

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (event != GTP_BUTTON_EVENT_PRESSED) {
		return;
	}

	if (color == GTP_BUTTON_UP) {
		if (player_vertical_pos < 6) {
			player_vertical_pos++;
		}

	} else if (color == GTP_BUTTON_DOWN) {
		if (player_vertical_pos > 0) {
			player_vertical_pos--;
		}
	}

	/* Small boolean hack to be locked on the last score display till as user press
	 * a button */
	if (game_is_finished) {
		game_is_finished = false;
	}
}

static inline void add_vehicule_at_actual_pos()
{
	if (player_vertical_pos >= 0 && player_vertical_pos <= 7) {
		buf[player_vertical_pos] |= 0x03;
		buf[player_vertical_pos + 1] |= 0x03;
	}
}

static inline void add_random_obstacles()
{
	static int8_t idx = 0;
	uint8_t rand = 0;
	static const uint8_t obstacle_len = 5;

	if (idx == 0) {
		rand = sys_rand8_get() % 8;
		LOG_INF("rand obstacle: %d", rand);
	}

	buf_obstacles[24 + rand] = 0x80;

	if (idx >= obstacle_len) {
		idx = -2;
	}

	idx++;
}

static inline void shift_column(const uint8_t column)
{
	if (column > 0) {
		for (uint8_t i = 0; i < 8; ++i) {
			const uint8_t tmp = buf_obstacles[8 * column + i] & 0x01;
			buf_obstacles[8 * (column - 1) + i] |= (tmp << 7);
		}
	}

	for (uint8_t i = 0; i < 8; ++i) {
		buf_obstacles[8 * column + i] = buf_obstacles[8 * column + i] >> 1;
	}
}

static inline void shift_all_obstacles()
{
	shift_column(0);
	shift_column(1);
	shift_column(2);
	shift_column(3);
}

static uint8_t detect_intersec_and_clear()
{
	/* The player vehicule is 4 pixels wide, a square.
	 * Simply check if any of these squares is on an obstacle.
	 * Returns the number of hits if this is the case. */

	uint8_t nb_hit = 0;

	if (buf_obstacles[player_vertical_pos] & 0x02) {
		buf_obstacles[player_vertical_pos] &= ~0x02;
		nb_hit++;
	}
	if (buf_obstacles[player_vertical_pos] & 0x01) {
		buf_obstacles[player_vertical_pos] &= ~0x01;
		nb_hit++;
	}
	if (buf_obstacles[player_vertical_pos + 1] & 0x02) {
		buf_obstacles[player_vertical_pos + 1] &= ~0x02;
		nb_hit++;
	}
	if (buf_obstacles[player_vertical_pos + 1] & 0x01) {
		buf_obstacles[player_vertical_pos + 1] &= ~0x01;
		nb_hit++;
	}
	return nb_hit;
}

static void play()
{
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);

	gtp_display_clear();
	k_msleep(100);

	memset(buf_obstacles, 0, sizeof(buf_obstacles));

	int i = 0;
	int score = 0;
	int total_hits = 0;

	while (1) {

		bool manage = i % 5 == 0 ? true : false;

		if (manage) {
			add_random_obstacles();
			memcpy(buf, buf_obstacles, sizeof(buf_obstacles));
			shift_all_obstacles();
		}

		i++;

		add_vehicule_at_actual_pos();
		gtp_display_print_buf(buf);

		if (manage) {
			const uint8_t nb = detect_intersec_and_clear();
			total_hits += nb;
			if (nb > 0) {
				if (game_mode == TRAFFIC_GAME_CATCH) {
					gtp_sound_good_short_bip();
				} else if (game_mode == TRAFFIC_GAME_ESCAPE) {
					gtp_sound_error_long_bip();
				}
			}
		}

		k_msleep(10);

		/* We stop catch game after a certain time ! */
		if (game_mode == TRAFFIC_GAME_CATCH && i >= 3000) {
			break;
		}

		/* We stop escape game after a certain amount of catch */
		if (game_mode == TRAFFIC_GAME_ESCAPE && total_hits >= 5) {
			break;
		}
	}

	gtp_display_clear();
	k_msleep(1000);

	if (game_mode == TRAFFIC_GAME_CATCH) {
		score = total_hits;
	} else if (game_mode == TRAFFIC_GAME_ESCAPE) {
		score = i;
	}

	gtp_game_display_score_int32(score);
}

void gtp_traffic_escape_game_init()
{
	k_sem_take(&traffic_escape_game_start, K_NO_WAIT);
}

const char *gtp_traffic_escape_game_get_menu_title()
{
	return menu_title_escape;
}

void gtp_traffic_escape_game_start()
{
	k_sem_give(&traffic_escape_game_start);
}

int gtp_traffic_escape_game_play()
{
	if (k_sem_take(&traffic_escape_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	game_mode = TRAFFIC_GAME_ESCAPE;
	play();
	gtp_game_wait_for_any_input(&game_is_finished);
	return GAME_WELL_FINISHED;
}

void gtp_traffic_catch_game_init()
{
	k_sem_take(&traffic_catch_game_start, K_NO_WAIT);
}

const char *gtp_traffic_catch_game_get_menu_title()
{
	return menu_title_catch;
}

void gtp_traffic_catch_game_start()
{
	k_sem_give(&traffic_catch_game_start);
}

int gtp_traffic_catch_game_play()
{
	if (k_sem_take(&traffic_catch_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	game_mode = TRAFFIC_GAME_CATCH;
	play();
	gtp_game_wait_for_any_input(&game_is_finished);
	return GAME_WELL_FINISHED;
}
