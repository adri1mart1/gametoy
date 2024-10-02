#include <gtp_buttons.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_buttons, CONFIG_GTPBUTTONS_LOG_LEVEL);

static on_gtp_buttons_event_cb_t on_gtp_buttons_event_cb;

#define ANTIBOUNCE_TIME K_MSEC(10)

/* Prototypes */
static void generic_pin_triggered(const struct gpio_dt_spec *pin);

/* Useful macros */

#define DECLARE_BUTTON(color)                                                                      \
	static const struct gpio_dt_spec button_##color##_pin =                                    \
		GPIO_DT_SPEC_GET(DT_ALIAS(button##color##pin), gpios);                             \
	static const struct gpio_dt_spec button_##color##_led =                                    \
		GPIO_DT_SPEC_GET(DT_ALIAS(button##color##led), gpios);                             \
	static struct gpio_callback button_##color##_cb;                                           \
                                                                                                   \
	static void antibounce_button_##color##_expired(struct k_work *)                           \
	{                                                                                          \
		generic_pin_triggered(&button_##color##_pin);                                      \
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
	if (gpio_pin_configure_dt(&button_##color##_led, GPIO_OUTPUT_ACTIVE) < 0) {                \
		return -ENODEV;                                                                    \
	}

DECLARE_BUTTON(red)
DECLARE_BUTTON(white)
DECLARE_BUTTON(blue)
DECLARE_BUTTON(yellow)
DECLARE_BUTTON(green)

static void generic_pin_triggered(const struct gpio_dt_spec *pin)
{
	if (gpio_pin_get_dt(pin) == 0) {
		LOG_INF("button %d unpressed", pin->pin);
	} else {
		LOG_INF("button %d pressed", pin->pin);
	}

	if (on_gtp_buttons_event_cb != NULL) {
		on_gtp_buttons_event_cb(GTP_BUTTON_COLOR_RED, GTP_BUTTON_EVENT_NONE);
	}
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

	bool toggle = true;

	while (1) {
		gpio_pin_set_dt(&button_red_led, toggle);
		gpio_pin_set_dt(&button_white_led, toggle);
		gpio_pin_set_dt(&button_blue_led, toggle);
		gpio_pin_set_dt(&button_yellow_led, toggle);
		gpio_pin_set_dt(&button_green_led, toggle);
		toggle = !toggle;
		k_msleep(1000);
	}
	return 0;
}

void gtp_buttons_set_led(const gtp_buttons_color_e color, const gtp_button_status_e status,
			 const int timeout_ms)
{
	if (status == GTP_BUTTON_STATUS_NONE) {
		return;
	}
}

void gtp_buttons_set_cb(on_gtp_buttons_event_cb_t cb)
{
	on_gtp_buttons_event_cb = cb;
}