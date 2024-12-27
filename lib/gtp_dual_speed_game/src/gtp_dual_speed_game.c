#include <gtp_dual_speed_game.h>
#include <gtp_buttons.h>
#include <gtp_display.h>
#include <gtp_game.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_dual_speed_game, CONFIG_GTPDUALSPEEDGAME_LOG_LEVEL);

K_SEM_DEFINE(dual_speed_game_start, 0, 1);

#define MAX_ROW 2

static const char *menu_title = "dual speed game";
static uint8_t buf[DISPLAY_WIDTH];
static uint8_t now_row = 0;
static uint8_t left_player_idx[MAX_ROW] = {0};
static uint8_t right_player_idx[MAX_ROW] = {0};
static bool game_is_finished = false;

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (event != GTP_BUTTON_EVENT_PRESSED) {
		return;
	}

	if (color == GTP_BUTTON_LEFT) {
		if (right_player_idx[now_row] > left_player_idx[now_row] + 1) {
			left_player_idx[now_row]++;
		} else {
			LOG_WRN("conflict left player");
			now_row++;
		}

	} else if (color == GTP_BUTTON_RIGHT) {
		if (left_player_idx[now_row] < right_player_idx[now_row] - 1) {
			right_player_idx[now_row]--;
		} else {
			LOG_WRN("conflict right player");
			now_row++;
		}
	}

	/* Small boolean hack to be locked on the last score display till as user press
	 * a button */
	if (game_is_finished) {
		game_is_finished = false;
		return;
	}

	if (now_row >= MAX_ROW) {
		game_is_finished = true;
	}
}

void gtp_dual_speed_game_init()
{
	k_sem_take(&dual_speed_game_start, K_NO_WAIT);
}

const char *gtp_dual_speed_game_get_menu_title()
{
	return menu_title;
}

void gtp_dual_speed_game_start()
{
	k_sem_give(&dual_speed_game_start);
}

static void prepare_initial_dots()
{
	memset(left_player_idx, 0, sizeof(left_player_idx));
	memset(right_player_idx, 31, sizeof(right_player_idx));
}

static void display_dots()
{
	memset(buf, 0, sizeof(buf));

	for (uint8_t i = 0; i < MAX_ROW; i++) {
		const uint8_t lcol = left_player_idx[i] / 8;
		buf[8 * lcol + i] |= 1 << left_player_idx[i] % 8;

		const uint8_t rcol = right_player_idx[i] / 8;
		buf[8 * rcol + i] |= 1 << right_player_idx[i] % 8;
	}

	gtp_display_print_buf(buf);
}

static void compute_score()
{
	uint8_t lscore = 0;
	uint8_t rscore = 0;

	for (uint8_t i = 0; i < MAX_ROW; i++) {
		lscore += left_player_idx[i];
		rscore += 31 - right_player_idx[i];
	}

	char fss[64] = {0};

	if (lscore > rscore) {
		snprintk(fss, sizeof(fss), "left player won %d > %d", lscore, rscore);
		LOG_WRN("left player won (%d > %d)", lscore, rscore);
	} else if (lscore < rscore) {
		snprintk(fss, sizeof(fss), "right player won %d < %d", lscore, rscore);
		LOG_WRN("right player won (%d < %d)", lscore, rscore);
	} else {
		snprintk(fss, sizeof(fss), "draw %d = %d", lscore, rscore);
		LOG_WRN("draw (%d = %d)", lscore, rscore);
	}
	gtp_display_print_sentence(fss, strlen(fss));
}

int gtp_dual_speed_game_play()
{
	if (k_sem_take(&dual_speed_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	game_is_finished = false;
	now_row = 0;
	gtp_display_clear();
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);
	k_msleep(1000);
	// gtp_game_countdown_to_play();

	prepare_initial_dots();
	gtp_display_print_buf(buf);

	while (game_is_finished == false) {
		display_dots();
		k_msleep(10);
	}

	compute_score();
	k_msleep(3000);

	gtp_game_wait_for_any_input(&game_is_finished);

	return GAME_WELL_FINISHED;
}
