#include <zephyr/kernel.h>
#include <gtp_buttons.h>
#include <gtp_menu.h>
#include <gtp_display.h>
#include <gtp_sound.h>
#include <gtp_reactivity_game.h>
#include <gtp_memory_game.h>
#include <gtp_simple_sound_game.h>
#include <gtp_traffic_game.h>
#include <gtp_dual_speed_game.h>
#include <app_version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	if (gtp_menu_is_menu_mode()) {

		if (event == GTP_BUTTON_EVENT_PRESSED) {
			/* Only up / down and validate buttons are handled in menu mode */
			const gtp_buttons_role_e role = (gtp_buttons_role_e)color;
			switch (role) {
			case GTP_BUTTON_UP_ROLE:
				gtp_menu_next();
				break;
			case GTP_BUTTON_DOWN_ROLE:
				gtp_menu_previous();
				break;
			case GTP_BUTTON_VALIDATE_ROLE:
				LOG_INF("Validate");
				gtp_display_set_menu_mode(false);
				gtp_menu_start_current_game();
				break;
			default:
				break;
			}
		}

	} else {
	}
}

void on_gtp_menu_event_cb(const char *menu_to_display)
{
	gtp_display_print_sentence(menu_to_display, strlen(menu_to_display));
}

int main()
{
	LOG_INF("starting game toy...");
	LOG_WRN("Software version: %s", APP_VERSION_STRING);

	gtp_reactivity_game_init();
	gtp_revert_reactivity_game_init();
	gtp_reactivity_phrase_game_init();
	gtp_simple_sound_game_init();
	gtp_memory_game_init();
	gtp_traffic_escape_game_init();
	gtp_traffic_catch_game_init();
	gtp_dual_speed_game_init();

	gtp_menu_init();
	gtp_menu_set_title(gtp_reactivity_game_get_menu_title(), gtp_reactivity_game_start);
	gtp_menu_set_title(gtp_reactivity_phrase_game_get_menu_title(),
			   gtp_reactivity_phrase_game_start);
	gtp_menu_set_title(gtp_revert_reactivity_game_get_menu_title(),
			   gtp_revert_reactivity_game_start);
	gtp_menu_set_title(gtp_simple_sound_game_get_menu_title(), gtp_simple_sound_game_start);
	gtp_menu_set_title(gtp_memory_game_get_menu_title(), gtp_memory_game_start);
	gtp_menu_set_title(gtp_traffic_escape_game_get_menu_title(), gtp_traffic_escape_game_start);
	gtp_menu_set_title(gtp_traffic_catch_game_get_menu_title(), gtp_traffic_catch_game_start);
	gtp_menu_set_title(gtp_sound_tell_it_on_the_mountain_get_menu_title(),
			   gtp_sound_tell_it_on_the_mountain_start);
	gtp_menu_set_title(gtp_sound_merry_christmas_get_menu_title(),
			   gtp_sound_a_merry_christmas_start);
	gtp_menu_set_title(gtp_dual_speed_game_get_menu_title(), gtp_dual_speed_game_start);
	gtp_menu_set_event_cb(on_gtp_menu_event_cb);

	gtp_display_init();
	gtp_sound_init();

	gtp_buttons_init();
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);

	gtp_display_set_menu_mode(true);
	gtp_menu_raise_cb();

	while (1) {
		k_msleep(1000);

		int ret = gtp_reactivity_game_play();
		ret |= gtp_revert_reactivity_game_play();
		ret |= gtp_reactivity_phrase_game_play();
		ret |= gtp_simple_sound_game_play();
		ret |= gtp_memory_game_play();
		ret |= gtp_traffic_escape_game_play();
		ret |= gtp_traffic_catch_game_play();
		ret |= gtp_dual_speed_game_play();

		/* song part */
		ret |= gtp_sound_play_tell_it_on_the_mountain();
		ret |= gtp_sound_play_merry_christmas();

		/* if we reach here, that mean the current game is done.
		 * back to menu mode again. */

		if (ret > 0) {
			LOG_WRN("a game has been finished");
			gtp_buttons_set_cb(on_gtp_buttons_event_cb);
			gtp_display_clear();
			gtp_buttons_set_all_leds_off();
			gtp_display_set_menu_mode(true);
			gtp_menu_raise_cb();
		}
	}
}
