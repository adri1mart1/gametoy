#include <gtp_reactivity_game.h>
#include <gtp_buttons.h>
#include <gtp_display.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_reactivity_game, CONFIG_GTPREACTIVITYGAME_LOG_LEVEL);

#define NUMBER_OF_ROUND 10

K_SEM_DEFINE(reactivity_game_start, 0, 1);

typedef struct {
	int64_t start;
	int64_t end;
} round_timing_t;

static int random_suite[NUMBER_OF_ROUND];
static round_timing_t round_timings[NUMBER_OF_ROUND];
static int round = 0;
K_SEM_DEFINE(next_round_semaphore, 0, 1);

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	LOG_INF("on_gtp_buttons_event_cb");
	if (color == random_suite[round] && event == GTP_BUTTON_EVENT_PRESSED) {
		gtp_buttons_set_leds(&random_suite[round], 1, GTP_BUTTON_STATUS_OFF, 0, 0, 0);
		round_timings[round].end = k_uptime_get();
		k_sem_give(&next_round_semaphore);
	}

	// TODO add penalties
}

void gtp_reactivity_game_init()
{
	LOG_WRN("reactivity_game_start mutex lock init");
	k_sem_take(&reactivity_game_start, K_NO_WAIT);
}

void gtp_reactivity_game_start()
{
	LOG_WRN("reactivity_game_start mutex unlock start");
	k_sem_give(&reactivity_game_start);
}

void gtp_reactivity_game_play()
{
	if (k_sem_take(&reactivity_game_start, K_NO_WAIT) != 0) {
		return;
	}

	gtp_display_clear();

	LOG_INF("GTP reactivity game started");

	gtp_buttons_set_cb(on_gtp_buttons_event_cb);
	memset(round_timings, 0, sizeof(round_timings));

	// sys_rand_get is not a true random generator.
	// Wait more or less time to vary the random numbers generated.
	k_msleep(70);

	sys_rand_get(random_suite, sizeof(random_suite));

	for (int i = 0; i < NUMBER_OF_ROUND; ++i) {
		random_suite[i] *= random_suite[i] < 0 ? -1 : 1;
		random_suite[i] = random_suite[i] % NUMBER_OF_BUTTONS;
	}

	LOG_INF("random_suite: %d %d %d %d %d %d %d %d %d %d", random_suite[0], random_suite[1],
		random_suite[2], random_suite[3], random_suite[4], random_suite[5], random_suite[6],
		random_suite[7], random_suite[8], random_suite[9]);

	while (round < NUMBER_OF_ROUND) {
		LOG_INF("round %d, color %d", round, random_suite[round]);
		gtp_buttons_set_leds(&random_suite[round], 1, GTP_BUTTON_STATUS_ON, 0, 0, 0);
		round_timings[round].start = k_uptime_get();
		k_sem_take(&next_round_semaphore, K_FOREVER);

		LOG_INF("round %d, time %lld", round,
			round_timings[round].end - round_timings[round].start);
		k_msleep(1000);

		round++;
	}

	// Compute total score
	int64_t total_time = 0;
	for (int i = 0; i < NUMBER_OF_ROUND; ++i) {
		total_time += round_timings[i].end - round_timings[i].start;
	}
	LOG_INF("Final score: %lld", total_time);
}
