
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <gtp_buttons.h>

#define LONG_PAUSE  k_msleep(10000);
#define SHORT_PAUSE k_msleep(5000);

void on_gtp_buttons_event_cb(const gtp_buttons_color_e color, const gtp_button_event_e event)
{
	LOG_INF("color: %d, event: %d", color, event);
}

int main()
{
	LOG_INF("Starting gtp buttons tests");

	gtp_buttons_init();
	gtp_buttons_set_cb(on_gtp_buttons_event_cb);

#if 0
	LONG_PAUSE

	LOG_INF("all led off");
	gtp_buttons_set_led(GTP_BUTTON_ALL_COLOR, GTP_BUTTON_STATUS_OFF, 0, 0, 0);
	SHORT_PAUSE

	LOG_INF("all led on");
	gtp_buttons_set_led(GTP_BUTTON_ALL_COLOR, GTP_BUTTON_STATUS_ON, 0, 0, 0);
	SHORT_PAUSE

	LOG_INF("all led off");
	gtp_buttons_set_led(GTP_BUTTON_ALL_COLOR, GTP_BUTTON_STATUS_OFF, 0, 0, 0);
	SHORT_PAUSE

#define TEST_LED_COLOR(color_str, color)                                                           \
	LOG_INF("led %s on", color_str);                                                           \
	gtp_buttons_set_led(GTP_BUTTON_##color##_COLOR, GTP_BUTTON_STATUS_ON, 0, 0, 0);            \
	SHORT_PAUSE                                                                                \
	LOG_INF("led %s off", color_str);                                                          \
	gtp_buttons_set_led(GTP_BUTTON_##color##_COLOR, GTP_BUTTON_STATUS_OFF, 0, 0, 0);           \
	SHORT_PAUSE                                                                                \
	LOG_INF("led %s blink for 6s", color_str);                                                 \
	gtp_buttons_set_led(GTP_BUTTON_##color##_COLOR, GTP_BUTTON_STATUS_BLINK, 500, 2000, 6000); \
	LONG_PAUSE

	TEST_LED_COLOR("red", RED)
	TEST_LED_COLOR("blue", BLUE)
	TEST_LED_COLOR("green", GREEN)
	TEST_LED_COLOR("yellow", YELLOW)
	TEST_LED_COLOR("white", WHITE)

	LOG_INF("all led off");
	gtp_buttons_set_led(GTP_BUTTON_ALL_COLOR, GTP_BUTTON_STATUS_OFF, 0, 0, 0);
	SHORT_PAUSE

	LOG_INF("all LEDs blink simultaneously");
	gtp_buttons_set_led(GTP_BUTTON_ALL_COLOR, GTP_BUTTON_STATUS_BLINK, 500, 1000, 8000);
#endif

#if 1
	LOG_INF("all LEDs blink");
	const int all_colors[] = {GTP_BUTTON_RED_COLOR, GTP_BUTTON_BLUE_COLOR,
				  GTP_BUTTON_GREEN_COLOR, GTP_BUTTON_YELLOW_COLOR,
				  GTP_BUTTON_WHITE_COLOR};

	gtp_buttons_set_leds(all_colors, sizeof(all_colors) / sizeof(all_colors[0]),
			     GTP_BUTTON_STATUS_BLINK, 100, 1000, 5000);
	LONG_PAUSE
#endif

#if 1
	LOG_INF("red white and green LEDs blink indefinitely ");
	const int red_white_green[] = {GTP_BUTTON_RED_COLOR, GTP_BUTTON_WHITE_COLOR,
				       GTP_BUTTON_GREEN_COLOR};
	gtp_buttons_set_leds(red_white_green, sizeof(red_white_green) / sizeof(red_white_green[0]),
			     GTP_BUTTON_STATUS_BLINK, 300, 600, 5000);
#endif

	while (1) {
		k_msleep(1000);
	}
}
