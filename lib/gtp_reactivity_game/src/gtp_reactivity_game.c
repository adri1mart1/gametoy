#include <gtp_reactivity_game.h>
#include <gtp_buttons.h>
#include <gtp_display.h>
#include <zephyr/kernel.h>
#include <gtp_game.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_reactivity_game, CONFIG_GTPREACTIVITYGAME_LOG_LEVEL);

#define NUMBER_OF_ROUND 10
#define PENALTY_TIME_MS 3000

K_SEM_DEFINE(reactivity_game_start, 0, 1);
K_SEM_DEFINE(revert_reactivity_game_start, 0, 1);
K_SEM_DEFINE(reactivity_phrase_game_start, 0, 1);

typedef struct {
	uint32_t start;
	uint32_t end;
} round_timing_t;

typedef enum {
	REACTIVITY_GAME_NORMAL = 0,
	REACTIVITY_GAME_INVERTED = 1,
	REACTIVITY_GAME_PHRASE = 2
} reactivity_game_mode_t;

static reactivity_game_mode_t game_mode = REACTIVITY_GAME_NORMAL;
static bool game_is_finished = false;
static round_timing_t round_timings[NUMBER_OF_ROUND];
static int round = 0;
static uint8_t *random_suite_ptr = NULL;
static const char *reactivity_game_menu_title = "reactivity game";
static const char *revert_reactivity_game_menu_title = "revert reactivity game";
static const char *reactivity_phrase_game_menu_title = "reactivity phrase game";

/* The order of the colors need to match with the button color enum ! */
static const char *color_phrases[] = {"red", "blue", "green", "yellow", "white"};

K_SEM_DEFINE(next_round_semaphore, 0, 1);

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (game_mode == REACTIVITY_GAME_PHRASE && event == GTP_BUTTON_EVENT_RELEASED) {
		gtp_buttons_set_led(color, GTP_BUTTON_STATUS_OFF);
		return;
	}

	if (event != GTP_BUTTON_EVENT_PRESSED) {
		return;
	}

	LOG_INF("on_gtp_buttons_event_cb");

	if ((game_mode == REACTIVITY_GAME_NORMAL && color == random_suite_ptr[round]) ||
	    (game_mode == REACTIVITY_GAME_INVERTED && color != random_suite_ptr[round]) ||
	    (game_mode == REACTIVITY_GAME_PHRASE && color == random_suite_ptr[round])) {

		if (game_mode == REACTIVITY_GAME_NORMAL || game_mode == REACTIVITY_GAME_INVERTED) {
			gtp_buttons_set_leds(&random_suite_ptr[round], 1, GTP_BUTTON_STATUS_OFF, 0,
					     0, 0);
		} else if (game_mode == REACTIVITY_GAME_PHRASE) {
			gtp_buttons_set_led(color, GTP_BUTTON_STATUS_ON);
		}

		round_timings[round].end = k_uptime_get_32();
		k_sem_give(&next_round_semaphore);

	} else {
		round_timings[round].start -= PENALTY_TIME_MS;
	}

	/* Small boolean hack to be locked on the last score display till as user press
	 * a button */
	if (game_is_finished) {
		game_is_finished = false;
	}
}

void gtp_reactivity_game_init()
{
	random_suite_ptr = gtp_game_get_random_suite_ptr();
	k_sem_take(&reactivity_game_start, K_NO_WAIT);
}

const char *gtp_reactivity_game_get_menu_title()
{
	return reactivity_game_menu_title;
}

void gtp_reactivity_game_start()
{
	k_sem_give(&reactivity_game_start);
}

static void prepare_game(const char *game_name)
{
	LOG_WRN("%s", game_name);
	gtp_game_countdown_to_play();
	LOG_INF("starting");

	gtp_buttons_set_cb(on_gtp_buttons_event_cb);
	memset(round_timings, 0, sizeof(round_timings));
	gtp_game_init_random_button_suite();

#if 0
	LOG_INF("random_suite: %d %d %d %d %d %d %d %d %d %d", random_suite_ptr[0],
		random_suite_ptr[1], random_suite_ptr[2], random_suite_ptr[3], random_suite_ptr[4],
		random_suite_ptr[5], random_suite_ptr[6], random_suite_ptr[7], random_suite_ptr[8],
		random_suite_ptr[9]);
#endif
}

static void play_game()
{
	while (round < NUMBER_OF_ROUND) {

		/* Display color */
		LOG_INF("round %d, color %d", round, random_suite_ptr[round]);

		if (game_mode == REACTIVITY_GAME_NORMAL || game_mode == REACTIVITY_GAME_INVERTED) {
			gtp_buttons_set_leds(&random_suite_ptr[round], 1, GTP_BUTTON_STATUS_ON, 0,
					     0, 0);
		} else if (game_mode == REACTIVITY_GAME_PHRASE) {
			gtp_display_print_sentence(color_phrases[random_suite_ptr[round]],
						   strlen(color_phrases[random_suite_ptr[round]]));
		}

		/* Take time snapshot */
		round_timings[round].start = k_uptime_get_32();

		/* Wait till user press a correct button */
		k_sem_take(&next_round_semaphore, K_FOREVER);

		LOG_INF("round %d, time %d", round,
			round_timings[round].end - round_timings[round].start);

		if (game_mode == REACTIVITY_GAME_PHRASE) {
			k_msleep(500);
			gtp_display_clear();
		}

		k_msleep(1000);
		round++;
	}
}

static void compute_score()
{
	int64_t total_time = 0;
	for (int i = 0; i < NUMBER_OF_ROUND; ++i) {
		total_time += round_timings[i].end - round_timings[i].start;
	}
	LOG_INF("final score: %lld", total_time);
	gtp_game_display_score_int64_millisec(total_time);
}

int gtp_reactivity_game_play()
{
	if (k_sem_take(&reactivity_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	prepare_game("reactivity game");
	game_mode = REACTIVITY_GAME_NORMAL;
	play_game();
	compute_score();
	gtp_game_wait_for_any_input(&game_is_finished);

	return GAME_WELL_FINISHED;
}

void gtp_revert_reactivity_game_init()
{
	k_sem_take(&revert_reactivity_game_start, K_NO_WAIT);
}

const char *gtp_revert_reactivity_game_get_menu_title()
{
	return revert_reactivity_game_menu_title;
}

void gtp_revert_reactivity_game_start()
{
	k_sem_give(&revert_reactivity_game_start);
}

int gtp_revert_reactivity_game_play()
{
	if (k_sem_take(&revert_reactivity_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	prepare_game("revert reactivity game");
	game_mode = REACTIVITY_GAME_INVERTED;
	play_game();
	compute_score();
	gtp_game_wait_for_any_input(&game_is_finished);

	return GAME_WELL_FINISHED;
}

void gtp_reactivity_phrase_game_init()
{
	k_sem_take(&reactivity_phrase_game_start, K_NO_WAIT);
}

const char *gtp_reactivity_phrase_game_get_menu_title()
{
	return reactivity_phrase_game_menu_title;
}

void gtp_reactivity_phrase_game_start()
{
	k_sem_give(&reactivity_phrase_game_start);
}

int gtp_reactivity_phrase_game_play()
{
	if (k_sem_take(&reactivity_phrase_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	prepare_game("reactivity phrase game");
	game_mode = REACTIVITY_GAME_PHRASE;
	play_game();
	compute_score();
	gtp_game_wait_for_any_input(&game_is_finished);

	return GAME_WELL_FINISHED;
}