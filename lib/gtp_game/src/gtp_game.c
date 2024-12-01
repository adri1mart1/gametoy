#include <gtp_game.h>
#include <gtp_display.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <gtp_buttons.h>
#include <string.h>

#define RANDOM_SUITE_MAX_LEN 50

static char fss[32] = {0};
static uint8_t random_suite[RANDOM_SUITE_MAX_LEN];

uint8_t *gtp_game_get_random_suite_ptr()
{
	return random_suite;
}

void gtp_game_countdown_to_play()
{
	static char buf[16] = {0};
	gtp_display_clear();
	snprintk(buf, sizeof(buf), "ready ?");
	gtp_display_print_sentence(buf, strlen(buf));
	k_msleep(1000);

	for (int i = 3; i > 0; --i) {
		snprintk(buf, sizeof(buf), "   %d", i);
		gtp_display_print_sentence(buf, strlen(buf));
		k_msleep(1000);
	}

	snprintk(buf, sizeof(buf), " play");
	gtp_display_print_sentence(buf, strlen(buf));
	k_msleep(1000);
}

void gtp_game_init_random_button_suite()
{
	sys_rand_get(random_suite, sizeof(random_suite));

	for (int i = 0; i < RANDOM_SUITE_MAX_LEN; ++i) {
		random_suite[i] *= random_suite[i] < 0 ? -1 : 1;
		random_suite[i] = random_suite[i] % NUMBER_OF_BUTTONS;
	}
}

void gtp_game_display_score_int64(const int64_t score)
{
	snprintf(fss, sizeof(fss), "score %lld", score);
	gtp_display_print_sentence(fss, strlen(fss));
}

void gtp_game_display_score_int32(const int score)
{
	snprintf(fss, sizeof(fss), "score %d", score);
	gtp_display_print_sentence(fss, strlen(fss));
}

void gtp_game_display_score_int64_millisec(const int64_t score)
{
	snprintf(fss, sizeof(fss), "score %lld ms", score);
	gtp_display_print_sentence(fss, strlen(fss));
}
