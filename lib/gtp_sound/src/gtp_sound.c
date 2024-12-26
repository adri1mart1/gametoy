#include <gtp_sound.h>
#include <gtp_game.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_sound, CONFIG_GTPSOUND_LOG_LEVEL);

#define STACK_SIZE            256
#define PRIORITY              5
#define SOUND_PERIOD_MS       10
#define DEFAULT_MUTEX_TIMEOUT K_MSEC(10)

static void sound_entry_point(void *, void *, void *);

K_THREAD_DEFINE(sound_tid, STACK_SIZE, sound_entry_point, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL,
		0);

K_MUTEX_DEFINE(sound_mutex);

K_SEM_DEFINE(tell_it_on_the_mountain_start, 0, 1);
K_SEM_DEFINE(merry_christmas_start, 0, 1);

typedef enum {
	SOUND_IDLE = 0,
	SOUND_TO_START,
	SOUND_ONGOING,
	SOUND_TO_STOP
} sound_state_e;

typedef struct {
	int remaining_duration_ms;
	uint16_t frequency;
	sound_state_e state;
} sound_state_t;

static sound_state_t sound_state;
static const uint16_t pwm_pulse_factor = 256u;
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwmled0));
static const char tell_it_on_the_mountain_title[] = "surprise song";
static const char merry_christmas_title[] = "merry christmas song";

static void sound_entry_point(void *, void *, void *)
{
	while (1) {

		const int ret = k_mutex_lock(&sound_mutex, DEFAULT_MUTEX_TIMEOUT);
		if (ret != 0) {
			LOG_ERR("mutex lock failed\n");
		} else {

			switch (sound_state.state) {
			case SOUND_TO_START:
				pwm_set_dt(&pwm_led0, PWM_HZ(sound_state.frequency),
					   PWM_HZ(sound_state.frequency) / pwm_pulse_factor);
				sound_state.remaining_duration_ms -= SOUND_PERIOD_MS;
				sound_state.state = SOUND_ONGOING;
				break;
			case SOUND_ONGOING:
				sound_state.remaining_duration_ms -= SOUND_PERIOD_MS;
				if (sound_state.remaining_duration_ms <= 0) {
					sound_state.state = SOUND_TO_STOP;
				}
				break;
			case SOUND_TO_STOP:
				pwm_set_dt(&pwm_led0, PWM_HZ(1024u), 0);
				sound_state.state = SOUND_IDLE;
				break;
			case SOUND_IDLE:
			default:
				break;
			}
		}

		k_mutex_unlock(&sound_mutex);

		k_msleep(SOUND_PERIOD_MS);
	}
}

void gtp_sound_init(void)
{
	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("PWM device %s is not ready\n", pwm_led0.dev->name);
	}

	k_sem_take(&tell_it_on_the_mountain_start, K_NO_WAIT);
	k_sem_take(&merry_christmas_start, K_NO_WAIT);
}

void gtp_sound_good_short_bip()
{
	gtp_game_sound_play_note(BIP_GOOD, 40);
}

void gtp_sound_good_long_bip()
{
	gtp_game_sound_play_note(BIP_GOOD, 300);
}

void gtp_sound_error_long_bip()
{
	gtp_game_sound_play_note(BIP_BAD, 300);
}

void gtp_game_sound_play_note(const uint16_t note, const int duration_ms)
{
	const int ret = k_mutex_lock(&sound_mutex, DEFAULT_MUTEX_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("mutex lock failed\n");
		return;
	}

	sound_state.frequency = note;
	sound_state.remaining_duration_ms = duration_ms;
	sound_state.state = SOUND_TO_START;

	k_mutex_unlock(&sound_mutex);
}

void gtp_game_sound_rest()
{
	const int ret = k_mutex_lock(&sound_mutex, DEFAULT_MUTEX_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("mutex lock failed\n");
		return;
	}

	sound_state.frequency = 0;
	sound_state.remaining_duration_ms = 0;
	sound_state.state = SOUND_TO_STOP;

	k_mutex_unlock(&sound_mutex);
}

static void play_song(const uint16_t melody[], const int durations_ms[], const int len)
{
	for (uint8_t i = 0; i < len; ++i) {
		gtp_game_sound_play_note(melody[i], durations_ms[i]);
		k_msleep(durations_ms[i] + 150);
	}
}

const char *gtp_sound_tell_it_on_the_mountain_get_menu_title()
{
	return tell_it_on_the_mountain_title;
}

void gtp_sound_tell_it_on_the_mountain_start()
{
	k_sem_give(&tell_it_on_the_mountain_start);
}

const char *gtp_sound_merry_christmas_get_menu_title()
{
	return merry_christmas_title;
}

void gtp_sound_a_merry_christmas_start()
{
	k_sem_give(&merry_christmas_start);
}

int gtp_sound_play_tell_it_on_the_mountain()
{
	if (k_sem_take(&tell_it_on_the_mountain_start, K_NO_WAIT) != 0) {
		return 0;
	}

	static const uint16_t melody[] = {
		NOTE_FS6, NOTE_FS6, NOTE_E6, NOTE_D6, NOTE_B5,           // Meas 03
		NOTE_A5,  NOTE_D6,                                       // Meas 04
		NOTE_E6,  NOTE_E6,  NOTE_E6, NOTE_D6, NOTE_E6,  NOTE_D6, // Meas 05
		NOTE_FS6, NOTE_A6,  NOTE_B6, NOTE_A6, NOTE_FS6, NOTE_E6, // Meas 06
		NOTE_FS6, NOTE_FS6, NOTE_E6, NOTE_D6, NOTE_B5,           // Meas 07
		NOTE_A5,  NOTE_D6,  NOTE_G6,                             // Meas 08
		NOTE_FS6, NOTE_D6,  NOTE_E6, NOTE_E6, NOTE_D6            // Meas 09
	};
	static const int noteDurations[] = {
		700, 200, 200, 200, 200,      // Meas 03
		700, 700,                     // Meas 04
		150, 200, 200, 200, 500, 500, // Meas 05
		500, 500, 200, 200, 200, 200, // Meas 06
		700, 200, 200, 200, 200,      // Meas 07
		700, 500, 300,                // Meas 08
		300, 150, 300, 300, 500       // Meas 09
	};

	play_song(melody, noteDurations, sizeof(melody) / sizeof(melody[0]));

	return SONG_WELL_FINISHED;
}

int gtp_sound_play_merry_christmas()
{
	if (k_sem_take(&merry_christmas_start, K_NO_WAIT) != 0) {
		return 0;
	}

	/* source: https://gmajormusictheory.org/Freebies/Sing/WeWishYouAMerry/WeWishYouAMerry.pdf
	 */

	static const uint16_t melody[] = {
		// Start
		NOTE_D6,                                        // Meas 01
		NOTE_G6,  NOTE_G6, NOTE_A6,  NOTE_G6, NOTE_FS6, // Meas 02
		NOTE_E6,  NOTE_E6, NOTE_E6,                     // Meas 03
		NOTE_A6,  NOTE_A6, NOTE_B6,  NOTE_A6, NOTE_G6,  // Meas 04
		NOTE_FS6, NOTE_D6, NOTE_D6,                     // Meas 05
		NOTE_B6,  NOTE_B6, NOTE_C6,  NOTE_B6, NOTE_A6,  // Meas 06
		NOTE_G6,  NOTE_E6, NOTE_D6,  NOTE_D6,           // Meas 07
		NOTE_E6,  NOTE_A6, NOTE_FS6,                    // Meas 08
		NOTE_G6,                                        // Meas 09
	};

	// Durées des notes (en millisecondes, divisées par la longueur des notes)
	// 4 = noire, 8 = croche, 2 = blanche
	static const int noteDurations[] = {
		// Start
		500,                     // Meas 01
		350, 250, 250, 250, 350, // Meas 02
		350, 300, 350,           // Meas 03
		350, 250, 250, 250, 250, // Meas 04
		350, 350, 300,           // Meas 05
		350, 250, 250, 250, 350, // Meas 06
		350, 350, 400, 400,      // Meas 07
		400, 450, 450,           // Meas 08
		900,                     // Meas 09
	};

	play_song(melody, noteDurations, sizeof(melody) / sizeof(melody[0]));

	return SONG_WELL_FINISHED;
}
