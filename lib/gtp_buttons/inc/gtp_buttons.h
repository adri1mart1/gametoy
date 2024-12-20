#ifndef GTP_BUTTONS_H__
#define GTP_BUTTONS_H__

#define MAX_DURATION_MS   65500
#define NUMBER_OF_BUTTONS 5

#include <zephyr/types.h>

typedef enum {
	GTP_BUTTON_NONE_COLOR = -1,
	GTP_BUTTON_RED_COLOR = 0,
	GTP_BUTTON_BLUE_COLOR = 1,
	GTP_BUTTON_GREEN_COLOR = 2,
	GTP_BUTTON_YELLOW_COLOR = 3,
	GTP_BUTTON_WHITE_COLOR = 4,
	GTP_BUTTON_ALL_COLOR = 255,
} gtp_buttons_color_e;

typedef enum {
	GTP_BUTTON_NONE_ROLE = -1,
	GTP_BUTTON_UP_ROLE = 3,       // yellow
	GTP_BUTTON_DOWN_ROLE = 2,     // green
	GTP_BUTTON_VALIDATE_ROLE = 4, // white
} gtp_buttons_role_e;

typedef enum {
	GTP_BUTTON_EVENT_NONE = 0,
	GTP_BUTTON_EVENT_PRESSED = 1,
	GTP_BUTTON_EVENT_RELEASED = 2,
} gtp_button_event_e;

typedef enum {
	GTP_BUTTON_STATUS_NONE = -1,
	GTP_BUTTON_STATUS_OFF = 0,
	GTP_BUTTON_STATUS_ON = 1,
	GTP_BUTTON_STATUS_BLINK = 2,
} gtp_button_status_e;

#define GTP_BUTTON_UP   GTP_BUTTON_YELLOW_COLOR
#define GTP_BUTTON_DOWN GTP_BUTTON_GREEN_COLOR

int gtp_buttons_init();

void gtp_buttons_set_all_leds_off();

void gtp_buttons_set_led(const gtp_buttons_color_e color, const gtp_button_status_e status);

void gtp_buttons_set_leds(const uint8_t *colors, const int color_size,
			  const gtp_button_status_e status, const int time_on_ms,
			  const int time_off_ms, const int duration_ms);

typedef void (*on_gtp_buttons_event_cb_t)(const gtp_buttons_color_e color,
					  const gtp_button_event_e event);

void gtp_buttons_set_cb(on_gtp_buttons_event_cb_t cb);

#endif // GTP_BUTTONS_H__