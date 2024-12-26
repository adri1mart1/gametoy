#include <gtp_game.h>
#include <gtp_simple_sound_game.h>
#include <gtp_buttons.h>
#include <gtp_sound.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_simple_sound_game, CONFIG_GTPSIMPLESOUNDGAME_LOG_LEVEL);

#include <zephyr/drivers/pwm.h>

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwmled0));

K_SEM_DEFINE(simple_sound_game_start, 0, 1);

static const char menu_title[] = "simple sound game";

static void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (event == GTP_BUTTON_EVENT_PRESSED) {

		gtp_buttons_set_led(color, GTP_BUTTON_STATUS_ON);

		switch (color) {
		case GTP_BUTTON_RED_COLOR:
			gtp_game_sound_play_note(NOTE_FS5, 999999);
			break;

		case GTP_BUTTON_BLUE_COLOR:
			gtp_game_sound_play_note(NOTE_D6, 999999);
			break;

		case GTP_BUTTON_GREEN_COLOR:
			gtp_game_sound_play_note(NOTE_E6, 999999);
			break;

		case GTP_BUTTON_YELLOW_COLOR:
			gtp_game_sound_play_note(NOTE_G6, 999999);
			break;

		case GTP_BUTTON_WHITE_COLOR:
			gtp_game_sound_play_note(NOTE_B6, 999999);
			break;

		default:
			break;
		}
	} else if (event == GTP_BUTTON_EVENT_RELEASED) {
		gtp_buttons_set_led(color, GTP_BUTTON_STATUS_OFF);
		gtp_game_sound_rest();
	}
}

void gtp_simple_sound_game_init()
{
	k_sem_take(&simple_sound_game_start, K_NO_WAIT);
}

const char *gtp_simple_sound_game_get_menu_title()
{
	return menu_title;
}

void gtp_simple_sound_game_start()
{
	k_sem_give(&simple_sound_game_start);
}

int gtp_simple_sound_game_play()
{
	if (k_sem_take(&simple_sound_game_start, K_NO_WAIT) != 0) {
		return 0;
	}

	if (!pwm_is_ready_dt(&pwm_led0)) {
		printk("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	static const char *title = "play music";
	gtp_display_print_sentence(title, strlen(title));

	gtp_buttons_set_cb(on_gtp_buttons_event_cb);

	while (1) {
		k_msleep(1000);
	}
	return 0;
}
