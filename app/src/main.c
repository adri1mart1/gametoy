#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <gtp_buttons.h>
#include <gtp_menu.h>
#include <gtp_display.h>
#include <gtp_reactivity_game.h>

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
				gtp_reactivity_game_start();
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

	gtp_reactivity_game_init();

	gtp_menu_init();
	gtp_menu_set_event_cb(on_gtp_menu_event_cb);

	gtp_display_init();

	gtp_buttons_init();
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);

	gtp_display_set_menu_mode(true);
	gtp_menu_raise_cb();

	while (1) {
		k_msleep(1000);
		gtp_reactivity_game_play();
	}
}
