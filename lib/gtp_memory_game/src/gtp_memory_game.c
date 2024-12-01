#include <gtp_memory_game.h>
#include <gtp_buttons.h>
#include <gtp_display.h>
#include <gtp_game.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_memory_game, CONFIG_GTPMEMORYGAME_LOG_LEVEL);

K_SEM_DEFINE(memory_game_start, 0, 1);

#define MAX_NUMBER_OF_MEMORY       50
#define BLINK_DURATION_MS          1000
#define BLINK_DURATION_INTERVAL_MS 100

static bool game_is_finished = false;
static int round_idx = 0;
static int move_idx = 0;
static bool sequence_complete = false;
static bool error_occured = false;
static uint8_t *random_suite_ptr = NULL;

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (event == GTP_BUTTON_EVENT_PRESSED) {
		gtp_buttons_set_led(color, GTP_BUTTON_STATUS_ON);

		if (color == random_suite_ptr[move_idx]) {
			/* player pressed the correct button, going to next move ! */
			++move_idx;
			LOG_WRN("correct button %d, next move %d", color, move_idx);

			if (move_idx >= round_idx) {
				LOG_WRN("seq complete");
				sequence_complete = true;
			}
		} else {
			/* play pressed the wrong button */
			LOG_WRN("wrong button %d expected [%d] -> ", color, move_idx,
				random_suite_ptr[move_idx]);
			error_occured = true;
		}

	} else if (event == GTP_BUTTON_EVENT_RELEASED) {
		gtp_buttons_set_led(color, GTP_BUTTON_STATUS_OFF);
	}

	// TODO add penalties

	/* Small boolean hack to be locked on the last score display till as user press
	 * a button */
	if (game_is_finished) {
		game_is_finished = false;
	}
}

void gtp_memory_game_init()
{
	random_suite_ptr = gtp_game_get_random_suite_ptr();
	k_sem_take(&memory_game_start, K_NO_WAIT);
}

void gtp_memory_game_start()
{
	k_sem_give(&memory_game_start);
}

int gtp_memory_game_play()
{
	if (k_sem_take(&memory_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	LOG_WRN("gtp_memory_game_play");
	gtp_game_countdown_to_play();

	gtp_buttons_set_all_leds_off();
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);
	gtp_game_init_random_button_suite();
#if 0
	LOG_INF("random_suite: %d %d %d %d %d %d %d %d %d %d", random_suite_ptr[0],
		random_suite_ptr[1], random_suite_ptr[2], random_suite_ptr[3], random_suite_ptr[4],
		random_suite_ptr[5], random_suite_ptr[6], random_suite_ptr[7], random_suite_ptr[8],
		random_suite_ptr[9]);
#endif
	round_idx = 1;
	move_idx = 0;
	game_is_finished = false;

	while (!game_is_finished) {

		sequence_complete = false;
		error_occured = false;

		/* Display a sequence a boutons */
		for (int i = 0; i < round_idx; ++i) {
			LOG_INF("display sequence of %d buttons", round_idx);
			gtp_buttons_set_leds(&random_suite_ptr[i], 1, GTP_BUTTON_STATUS_BLINK, 1000,
					     0, BLINK_DURATION_MS);
			k_msleep(BLINK_DURATION_MS + BLINK_DURATION_INTERVAL_MS);
		}

		LOG_INF("Wait till seq is done or error");
		/* Wait till all sequence is done or an error ! */
		while (!sequence_complete && !error_occured) {
			k_msleep(10);
		}

		if (error_occured) {
			LOG_INF("error occured, final score: %d", round_idx);
			gtp_game_display_score_int32(round_idx);
			k_msleep(2000);
			game_is_finished = true;
		}

		if (sequence_complete) {
			LOG_INF("Next round, %d buttons to memorised", round_idx);
			char buf[] = "correct";
			gtp_display_print_sentence(buf, strlen(buf));
			k_msleep(2000);
			gtp_display_clear();
			++round_idx;
			move_idx = 0;
		}
	}

	game_is_finished = true;
	while (game_is_finished) {
		k_msleep(10);
	}

	return GAME_WELL_FINISHED;
}