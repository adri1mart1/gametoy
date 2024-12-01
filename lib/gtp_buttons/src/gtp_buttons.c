#include <gtp_buttons.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_buttons, CONFIG_GTPBUTTONS_LOG_LEVEL);

static on_gtp_buttons_event_cb_t on_gtp_buttons_event_cb;

#define ANTIBOUNCE_TIME   K_MSEC(10)
#define STACK_SIZE        256
#define PRIORITY          5
#define NUMBER_OF_BUTTONS 5
#define BLINKY_PERIOD_MS  50

/* Prototypes */
static void blinky_entry_point(void *, void *, void *);
static void generic_pin_triggered_work(const struct gpio_dt_spec *pin,
				       const gtp_buttons_color_e color);
static void reset_led_blink_state(const gtp_buttons_color_e color, const int led_new_value);

/* Useful macros */

#define DECLARE_BUTTON(color, COLOR)                                                               \
	static const struct gpio_dt_spec button_##color##_pin =                                    \
		GPIO_DT_SPEC_GET(DT_ALIAS(button##color##pin), gpios);                             \
	static const struct gpio_dt_spec button_##color##_led =                                    \
		GPIO_DT_SPEC_GET(DT_ALIAS(button##color##led), gpios);                             \
	static struct gpio_callback button_##color##_cb;                                           \
                                                                                                   \
	static void antibounce_button_##color##_expired(struct k_work *)                           \
	{                                                                                          \
		generic_pin_triggered_work(&button_##color##_pin, GTP_BUTTON_##COLOR##_COLOR);     \
	}                                                                                          \
                                                                                                   \
	static K_WORK_DELAYABLE_DEFINE(antibounce_button_##color##_work,                           \
				       antibounce_button_##color##_expired);                       \
                                                                                                   \
	static void button_##color##_pin_triggered(const struct device *dev,                       \
						   struct gpio_callback *cb, uint32_t pins)        \
	{                                                                                          \
		k_work_reschedule(&antibounce_button_##color##_work, ANTIBOUNCE_TIME);             \
	}

#define CONFIGURE_BUTTON(color)                                                                    \
	if (!gpio_is_ready_dt(&button_##color##_pin)) {                                            \
		LOG_ERR("gpio not ready");                                                         \
		return -EIO;                                                                       \
	}                                                                                          \
                                                                                                   \
	if (gpio_pin_configure_dt(&button_##color##_pin, GPIO_INPUT) < 0) {                        \
		return -ENODEV;                                                                    \
	}                                                                                          \
                                                                                                   \
	ret = gpio_pin_interrupt_configure_dt(&button_##color##_pin, GPIO_INT_EDGE_BOTH);          \
                                                                                                   \
	if (ret != 0) {                                                                            \
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret,             \
			button_##color##_pin.port->name, button_##color##_pin.pin);                \
		return ret;                                                                        \
	}                                                                                          \
                                                                                                   \
	gpio_init_callback(&button_##color##_cb, button_##color##_pin_triggered,                   \
			   BIT(button_##color##_pin.pin));                                         \
	ret = gpio_add_callback(button_##color##_pin.port, &button_##color##_cb);                  \
	if (ret) {                                                                                 \
		LOG_ERR("Failed to add GPIO callback");                                            \
	}

#define CONFIGURE_LED_BUTTON(color)                                                                \
	if (!gpio_is_ready_dt(&button_##color##_led)) {                                            \
		LOG_ERR("gpio not ready");                                                         \
		return -EIO;                                                                       \
	}                                                                                          \
                                                                                                   \
	if (gpio_pin_configure_dt(&button_##color##_led, GPIO_OUTPUT_INACTIVE) < 0) {              \
		return -ENODEV;                                                                    \
	}

DECLARE_BUTTON(red, RED)
DECLARE_BUTTON(white, WHITE)
DECLARE_BUTTON(blue, BLUE)
DECLARE_BUTTON(yellow, YELLOW)
DECLARE_BUTTON(green, GREEN)

K_THREAD_DEFINE(blinky_tid, STACK_SIZE, blinky_entry_point, NULL, NULL, NULL, PRIORITY, K_ESSENTIAL,
		0);

K_MUTEX_DEFINE(blinky_mutex);

typedef struct {
	int remaining_duration_ms;
	int const_time_on;
	int const_time_off;
	int remaining_state_time;
	int state;
	const struct gpio_dt_spec *pin;
	bool blink_mode_on;
	gtp_buttons_color_e color;
} led_state_t;

static const uint8_t all_leds[] = {0, 1, 2, 3, 4};
static led_state_t led_state[NUMBER_OF_BUTTONS];

BUILD_ASSERT(GTP_BUTTON_RED_COLOR < sizeof(led_state) / sizeof(led_state[0]));
BUILD_ASSERT(GTP_BUTTON_BLUE_COLOR < sizeof(led_state) / sizeof(led_state[0]));
BUILD_ASSERT(GTP_BUTTON_GREEN_COLOR < sizeof(led_state) / sizeof(led_state[0]));
BUILD_ASSERT(GTP_BUTTON_YELLOW_COLOR < sizeof(led_state) / sizeof(led_state[0]));
BUILD_ASSERT(GTP_BUTTON_WHITE_COLOR < sizeof(led_state) / sizeof(led_state[0]));

static void evaluate_led_state(led_state_t *led_state)
{
	if (led_state->blink_mode_on == false) {
		return;
	}

	// checking end blinking condition
	if (led_state->remaining_duration_ms <= 0) {
		reset_led_blink_state(led_state->color, 0);

	} else {
		gpio_pin_set_dt(led_state->pin, led_state->state);
		led_state->remaining_duration_ms -= BLINKY_PERIOD_MS;
		led_state->remaining_state_time -= BLINKY_PERIOD_MS;

		if (led_state->state == 1) {
			if (led_state->remaining_state_time <= 0) {
				// switch state from on to off!
				led_state->state = 0;
				led_state->remaining_state_time = led_state->const_time_off;
			}
		} else {
			if (led_state->remaining_state_time <= 0) {
				// switch state from off to on!
				led_state->state = 1;
				led_state->remaining_state_time = led_state->const_time_on;
			}
		}
	}
}

static void blinky_entry_point(void *, void *, void *)
{
	while (1) {

		k_mutex_lock(&blinky_mutex, K_FOREVER);

		for (int i = 0; i < NUMBER_OF_BUTTONS; ++i) {
			evaluate_led_state(&led_state[i]);
		}

		k_mutex_unlock(&blinky_mutex);

		k_sleep(K_MSEC(BLINKY_PERIOD_MS));
	}
}

static void generic_pin_triggered_work(const struct gpio_dt_spec *pin,
				       const gtp_buttons_color_e color)
{
	const gtp_button_event_e evt =
		gpio_pin_get_dt(pin) == 1 ? GTP_BUTTON_EVENT_PRESSED : GTP_BUTTON_EVENT_RELEASED;

	if (on_gtp_buttons_event_cb != NULL) {
		on_gtp_buttons_event_cb(color, evt);
	}
}

static void reset_led_blink_state(const gtp_buttons_color_e color, const int led_new_value)
{
	/* this function must be called under mutex protection only */
	gpio_pin_set_dt(led_state[color].pin, led_new_value);
	led_state[color].state = 0;
	led_state[color].remaining_duration_ms = 0;
	led_state[color].const_time_on = 0;
	led_state[color].const_time_off = 0;
	led_state[color].remaining_state_time = 0;
	led_state[color].blink_mode_on = false;
}

int gtp_buttons_init()
{
	int ret;

	CONFIGURE_BUTTON(red)
	CONFIGURE_BUTTON(white)
	CONFIGURE_BUTTON(blue)
	CONFIGURE_BUTTON(yellow)
	CONFIGURE_BUTTON(green)

	CONFIGURE_LED_BUTTON(red)
	CONFIGURE_LED_BUTTON(white)
	CONFIGURE_LED_BUTTON(blue)
	CONFIGURE_LED_BUTTON(yellow)
	CONFIGURE_LED_BUTTON(green)

	memset(&led_state, 0, sizeof(led_state));
	led_state[GTP_BUTTON_RED_COLOR].pin = &button_red_led;
	led_state[GTP_BUTTON_RED_COLOR].color = GTP_BUTTON_RED_COLOR;
	led_state[GTP_BUTTON_BLUE_COLOR].pin = &button_blue_led;
	led_state[GTP_BUTTON_BLUE_COLOR].color = GTP_BUTTON_BLUE_COLOR;
	led_state[GTP_BUTTON_GREEN_COLOR].pin = &button_green_led;
	led_state[GTP_BUTTON_GREEN_COLOR].color = GTP_BUTTON_GREEN_COLOR;
	led_state[GTP_BUTTON_YELLOW_COLOR].pin = &button_yellow_led;
	led_state[GTP_BUTTON_YELLOW_COLOR].color = GTP_BUTTON_YELLOW_COLOR;
	led_state[GTP_BUTTON_WHITE_COLOR].pin = &button_white_led;
	led_state[GTP_BUTTON_WHITE_COLOR].color = GTP_BUTTON_WHITE_COLOR;
	reset_led_blink_state(GTP_BUTTON_RED_COLOR, 0);
	reset_led_blink_state(GTP_BUTTON_BLUE_COLOR, 0);
	reset_led_blink_state(GTP_BUTTON_GREEN_COLOR, 0);
	reset_led_blink_state(GTP_BUTTON_YELLOW_COLOR, 0);
	reset_led_blink_state(GTP_BUTTON_WHITE_COLOR, 0);

	return 0;
}

static inline void configure_start_blink(const int idx, const int time_on_ms, const int time_off_ms,
					 const int duration_ms)
{
	led_state[idx].const_time_on = time_on_ms;
	led_state[idx].const_time_off = time_off_ms;
	// if start_state is 0, remaining_state_time must be the time off.
	// else, remaining_state_time must be the time on.
	led_state[idx].remaining_state_time = time_on_ms;
	led_state[idx].remaining_duration_ms = duration_ms;
	led_state[idx].state = 1;
	led_state[idx].blink_mode_on = true;
}

void gtp_buttons_set_all_leds_off()
{
	gtp_buttons_set_leds(all_leds, sizeof(all_leds) / sizeof(all_leds[0]),
			     GTP_BUTTON_STATUS_OFF, 0, 0, 0);
}

void gtp_buttons_set_led(const gtp_buttons_color_e color, const gtp_button_status_e status)
{
	if (status != GTP_BUTTON_STATUS_OFF && status != GTP_BUTTON_STATUS_ON) {
		LOG_ERR("function incompatible with this status: %d", status);
		return;
	}

	if (color == GTP_BUTTON_NONE_COLOR) {
		return;
	}

	if (color == GTP_BUTTON_ALL_COLOR) {
		gtp_buttons_set_leds(all_leds, sizeof(all_leds) / sizeof(all_leds[0]), status, 0, 0,
				     0);
		return;
	}

	gtp_buttons_set_leds((const uint8_t *)&color, 1, status, 0, 0, 0);
}

void gtp_buttons_set_leds(const uint8_t *colors, const int color_size,
			  const gtp_button_status_e status, const int time_on_ms,
			  const int time_off_ms, const int duration_ms)
{
	__ASSERT(colors != NULL, "invalid colors", NULL);
	__ASSERT(color_size > 0, "invalid color size", NULL);
	for (int i = 0; i < color_size; ++i) {
		__ASSERT(colors[i] >= 0, "invalid color", NULL);
		__ASSERT(colors[i] < NUMBER_OF_BUTTONS, "invalid color", NULL);
	}
	__ASSERT(duration_ms % BLINKY_PERIOD_MS == 0, "invalid duration", NULL);
	__ASSERT(time_on_ms % BLINKY_PERIOD_MS == 0, "invalid time on", NULL);
	__ASSERT(time_off_ms % BLINKY_PERIOD_MS == 0, "invalid time off", NULL);
	__ASSERT(color != GTP_BUTTON_NONE_COLOR, "invalid color", NULL);
	__ASSERT(status != GTP_BUTTON_STATUS_NONE, "invalid status", NULL);

	k_mutex_lock(&blinky_mutex, K_FOREVER);

	for (int color_idx = 0; color_idx < color_size; ++color_idx) {
		if (status == GTP_BUTTON_STATUS_ON || status == GTP_BUTTON_STATUS_OFF) {
			if (colors[color_idx] == GTP_BUTTON_ALL_COLOR) {
				for (int i = 0; i < NUMBER_OF_BUTTONS; ++i) {
					reset_led_blink_state(i, status);
				}
			} else {
				reset_led_blink_state(colors[color_idx], status);
			}
		}

		if (status == GTP_BUTTON_STATUS_BLINK) {
			if (colors[color_idx] == GTP_BUTTON_ALL_COLOR) {
				for (int i = 0; i < NUMBER_OF_BUTTONS; ++i) {
					configure_start_blink(i, time_on_ms, time_off_ms,
							      duration_ms);
				}
			} else {
				configure_start_blink(colors[color_idx], time_on_ms, time_off_ms,
						      duration_ms);
			}
		}
	}

	k_mutex_unlock(&blinky_mutex);
}

void gtp_buttons_set_cb(on_gtp_buttons_event_cb_t cb)
{
	on_gtp_buttons_event_cb = cb;
}
