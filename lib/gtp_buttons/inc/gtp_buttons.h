#ifndef GTP_BUTTONS_H__
#define GTP_BUTTONS_H__

typedef enum {
	GTP_BUTTON_COLOR_NONE = 0,
	GTP_BUTTON_COLOR_RED = 1,
	GTP_BUTTON_COLOR_BLUE = 2,
	GTP_BUTTON_COLOR_GREEN = 3,
	GTP_BUTTON_COLOR_YELLOW = 4,
	GTP_BUTTON_COLOR_WHITE = 5,
} gtp_buttons_color_e;

typedef enum {
	GTP_BUTTON_EVENT_NONE = 0,
	GTP_BUTTON_EVENT_PRESSED = 1,
	GTP_BUTTON_EVENT_RELEASED = 2,
} gtp_button_event_e;

typedef enum {
	GTP_BUTTON_STATUS_NONE = 0,
	GTP_BUTTON_SATUS_ON = 1,
	GTP_BUTTON_STATUS_OFF = 2,
	GTP_BUTTON_STATUS_BLINK = 3,
} gtp_button_status_e;

int gtp_buttons_init();

void gtp_buttons_set_led(const gtp_buttons_color_e color, const gtp_button_status_e status,
			 const int timeout_ms);

typedef void (*on_gtp_buttons_event_cb_t)(const gtp_buttons_color_e color,
					  const gtp_button_event_e event);

void gtp_buttons_set_cb(on_gtp_buttons_event_cb_t cb);

#endif // GTP_BUTTONS_H__