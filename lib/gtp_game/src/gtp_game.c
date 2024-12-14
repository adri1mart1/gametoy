#include <gtp_game.h>
#include <gtp_display.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/random/random.h>
#include <gtp_buttons.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_game, CONFIG_GTPGAME_LOG_LEVEL);

#define RANDOM_SUITE_MAX_LEN 50

static char fss[32] = {0};
static uint8_t random_suite[RANDOM_SUITE_MAX_LEN];
static const int pwm_pulse_factor = 256u;
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwmled0));

uint8_t *gtp_game_get_random_suite_ptr()
{
	return random_suite;
}

void gtp_game_init()
{
	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}
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

static void play_note_expired(struct k_work *work)
{
	pwm_set_dt(&pwm_led0, PWM_HZ(REST), PWM_HZ(REST) / pwm_pulse_factor);
}

K_WORK_DELAYABLE_DEFINE(play_note_work, play_note_expired);

void gtp_game_sound_play_note(const int note, const int duration_ms)
{
	pwm_set_dt(&pwm_led0, PWM_HZ(note), PWM_HZ(note) / pwm_pulse_factor);

	if (duration_ms > 0) {
		k_work_reschedule(&play_note_work, K_MSEC(duration_ms));
	}
}

void gtp_game_wait_for_any_input(bool *boolean)
{
	*boolean = true;
	while (*boolean) {
		k_msleep(10);
	}
}
