#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

enum corner {
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT
};

typedef void (*fill_buffer)(enum corner corner, uint8_t grey, uint8_t *buf,
			    size_t buf_size);


static void fill_buffer_argb8888(enum corner corner, uint8_t grey, uint8_t *buf,
				 size_t buf_size)
{
	uint32_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0x00FF0000u;
		break;
	case TOP_RIGHT:
		color = 0x0000FF00u;
		break;
	case BOTTOM_RIGHT:
		color = 0x000000FFu;
		break;
	case BOTTOM_LEFT:
		color = grey << 16 | grey << 8 | grey;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 4) {
		*((uint32_t *)(buf + idx)) = color;
	}
}

static void fill_buffer_rgb888(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint32_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0x00FF0000u;
		break;
	case TOP_RIGHT:
		color = 0x0000FF00u;
		break;
	case BOTTOM_RIGHT:
		color = 0x000000FFu;
		break;
	case BOTTOM_LEFT:
		color = grey << 16 | grey << 8 | grey;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 3) {
		*(buf + idx + 0) = color >> 16;
		*(buf + idx + 1) = color >> 8;
		*(buf + idx + 2) = color >> 0;
	}
}

static uint16_t get_rgb565_color(enum corner corner, uint8_t grey)
{
	uint16_t color = 0;
	uint16_t grey_5bit;

	switch (corner) {
	case TOP_LEFT:
		color = 0xF800u;
		break;
	case TOP_RIGHT:
		color = 0x07E0u;
		break;
	case BOTTOM_RIGHT:
		color = 0x001Fu;
		break;
	case BOTTOM_LEFT:
		grey_5bit = grey & 0x1Fu;
		/* shift the green an extra bit, it has 6 bits */
		color = grey_5bit << 11 | grey_5bit << (5 + 1) | grey_5bit;
		break;
	}
	return color;
}

static void fill_buffer_rgb565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(buf + idx + 0) = (color >> 8) & 0xFFu;
		*(buf + idx + 1) = (color >> 0) & 0xFFu;
	}
}

static void fill_buffer_bgr565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(uint16_t *)(buf + idx) = color;
	}
}

static void fill_buffer_mono(enum corner corner, uint8_t grey,
			     uint8_t black, uint8_t white,
			     uint8_t *buf, size_t buf_size)
{
	uint16_t color;

	switch (corner) {
	case BOTTOM_LEFT:
		color = (grey & 0x01u) ? white : black;
		break;
	default:
		color = black;
		break;
	}

    LOG_INF("color: %d", color);
	memset(buf, white, buf_size);
}

static inline void fill_buffer_mono01(enum corner corner, uint8_t grey,
				      uint8_t *buf, size_t buf_size)
{
	fill_buffer_mono(corner, grey, 0x00u, 0xFFu, buf, buf_size);
}

int main(void)
{
	size_t x;
	size_t y;
	size_t rect_w;
	size_t rect_h;
	size_t h_step;
	size_t scale;
	size_t grey_count;
	uint8_t bg_color;
	uint8_t *buf;
	int32_t grey_scale_sleep;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;
	fill_buffer fill_buffer_fnc = NULL;

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.",
			display_dev->name);
		return 0;
	}

	LOG_INF("Display sample for %s", display_dev->name);
	display_get_capabilities(display_dev, &capabilities);
    LOG_INF("x_resolution: %d", capabilities.x_resolution);
    LOG_INF("y_resolution: %d", capabilities.y_resolution);

    rect_w = capabilities.x_resolution;
    rect_h = capabilities.y_resolution;

    buf_size = rect_h;

    /* capabilities.current_pixel_format */
    bg_color = 0x00u;
    fill_buffer_fnc = fill_buffer_mono01;

	LOG_INF("buf_size = %u", buf_size);
	buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting sample.");
		return 0;
	}

	(void)memset(buf, bg_color, buf_size);

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = capabilities.y_resolution;

    // a -> 0e0909090e000000
    // b -> 0709090907010100
    // c -> 0e0101010e000000
    // d -> 0e0909090e080800
    // e -> 0e010f0906000000

    // const char letter_a[] = {0x00, 0x6e, 0x33, 0x3e, 0x30, 0x1e, 0x00, 0x00};
    const char letter_a[] = {0x00, 0x1c, 0x12, 0x12, 0x12, 0x1c, 0x00, 0x00};
    // const char letter_b[] = {0x00, 0x3b, 0x66, 0x66, 0x3e, 0x06, 0x06, 0x07};
    const char letter_b[] = {0x00, 0x07, 0x09, 0x09, 0x09, 0x07, 0x01, 0x01};
    // const char letter_c[] = {0x00, 0x1e, 0x33, 0x03, 0x33, 0x1e, 0x00, 0x00};
    const char letter_c[] = {0x0e, 0x01, 0x01, 0x01, 0x0e, 0x00, 0x00, 0x00};

    // const uint64_t IMAGES[] = { 0x003f260c193f0000 };
    // const int IMAGES_LEN = sizeof(IMAGES)/8;

    memcpy(buf, &IMAGES[0], sizeof(IMAGES[0]));
    memcpy(buf + 8, &letter_b, sizeof(letter_b));
    memcpy(buf + 16, &letter_c, sizeof(letter_c));

    #if 0

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		/*
		 * Tweaking the height value not to draw outside of the display.
		 * It is required when using a monochrome display whose vertical
		 * resolution can not be divided by 8.
		 */
		if ((capabilities.y_resolution - idx) < h_step) {
			buf_desc.height = (capabilities.y_resolution - idx);
		}
		display_write(display_dev, 0, idx, &buf_desc, buf);
	}

	buf_desc.pitch = rect_w;
	buf_desc.width = rect_w;
	buf_desc.height = rect_h;

	fill_buffer_fnc(TOP_LEFT, 0, buf, buf_size);
	x = 0;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	fill_buffer_fnc(TOP_RIGHT, 0, buf, buf_size);
	x = capabilities.x_resolution - rect_w;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	fill_buffer_fnc(BOTTOM_RIGHT, 0, buf, buf_size);
	x = capabilities.x_resolution - rect_w;
	y = capabilities.y_resolution - rect_h;
	display_write(display_dev, x, y, &buf_desc, buf);

	display_blanking_off(display_dev);

	grey_count = 0;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, buf_size);
		display_write(display_dev, x, y, &buf_desc, buf);
		++grey_count;
		k_msleep(grey_scale_sleep);
	}

    #endif

    /* init all to zeros */
    /*
    fill_buffer_fnc(TOP_LEFT, 0, buf, buf_size);
	x = 0;
	y = 0;

    */

    display_write(display_dev, 0, 0, &buf_desc, buf);

    while (1) {


        k_msleep(1000);
    }
	return 0;
}
