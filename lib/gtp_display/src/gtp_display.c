#include "gtp_display.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtpdisplay, LOG_LEVEL_DBG);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static struct display_capabilities capabilities;
static struct display_buffer_descriptor buf_desc;

#define STACK_SIZE 512
#define PRIORITY   5

#define DISPLAY_WIDTH 32

/* When the menu is ON with cursors, cursors are 5 LEDs wide + 1 column of space.
 * So the cursor overall takes 6 columns of LEDs, nothing should override that. */
#define DISPLAY_WIDTH_MENU_ON (DISPLAY_WIDTH - 6)

// the display buffer that holds data
static uint8_t buf[DISPLAY_WIDTH];

static void gtp_display_entry_point(void *, void *, void *);

/* I'am using a dot matrix 8x32 display. It's a 8x8 dot matrix chained using
 * 4 modules in series.
 * In the end, the 8x32 display needs to be horizontal for better reading.
 * We need to choose one position, the reference [0;0] can be in any corner
 * and the physical position of the display can be in two sides.
 * I chose to have the connector on the left.
 * We using the zephyr display API using a buffer, the buf[0] is the lower
 * left pixel so that will be my reference.
 *  ^
 *  |
 *  |
 * [0;0]--------->
 *
 * I will use a combination of masks to display symbols easily. Any character,
 * digits, symbols will be a single mask that can be applied to the buffer
 * respecting buffer boundaries.
 *
 * The whole matrix 8x32 is represented as a buffer of 32 bytes. One pixel
 * is a single bit.
 * The representations is a follows:
 *  _______________________________________
 * | byte 07 | byte 15 | byte 23 | byte 31 | ^
 * | ..................................... | |
 * | byte 02 | byte 10 | byte 18 | byte 26 | | 8 dots LED
 * | byte 01 | byte 09 | byte 17 | byte 25 | |
 * | byte 00 | byte 08 | byte 16 | byte 24 | |
 * |_______________________________________| v
 * <------------- 32 dots LED ------------->
 *
 * Each byte is also inverted, that mean we have:
 * bit0, bit1, bit2, ... bit7 horizontally.
 */

K_THREAD_DEFINE(gtp_display_tid, STACK_SIZE, gtp_display_entry_point, NULL, NULL, NULL, PRIORITY,
		K_ESSENTIAL, 0);

K_MUTEX_DEFINE(gtp_display_mutex);

#define GTP_DISPLAY_EVENT_NEW_WORD             0x01u
#define GTP_DISPLAY_EVENT_SHIFT_TEXT           0x02u
#define GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_ON  0x04u
#define GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_OFF 0x08u

#define GTP_DISPLAY_ALL_EVENTS_MASK                                                                \
	(GTP_DISPLAY_EVENT_NEW_WORD | GTP_DISPLAY_EVENT_SHIFT_TEXT |                               \
	 GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_ON | GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_OFF)

K_EVENT_DEFINE(gtp_display_event);

static int min_x_display_area = 0;
static int max_x_display_area = DISPLAY_WIDTH - 1;
static bool menu_mode = false;

#define SENTENCE_SIZE 64
static char sentence[SENTENCE_SIZE] = {0};

typedef struct {
	char symbol;
	char mask[8];
	int width;
} display_symbol_t;

#define ASCII_OFFSET  ' '
#define ALPHABET_SIZE 26

static char up_arrow_mask[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x70, 0x20};
static char down_arrow_mask[8] = {0x20, 0x70, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00};

const display_symbol_t symbols[] = {
	{' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1}, // space

	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0}, // padding
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},

	{'0', {0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, 0x00}, 4},
	{'1', {0x07, 0x02, 0x02, 0x02, 0x02, 0x03, 0x02, 0x00}, 3},
	{'2', {0x0f, 0x01, 0x02, 0x04, 0x08, 0x09, 0x06, 0x00}, 4},
	{'3', {0x06, 0x09, 0x08, 0x06, 0x08, 0x09, 0x06, 0x00}, 4},
	{'4', {0x08, 0x08, 0x1f, 0x09, 0x0a, 0x0c, 0x08, 0x00}, 5},
	{'5', {0x06, 0x09, 0x08, 0x0f, 0x01, 0x01, 0x0f, 0x00}, 4},
	{'6', {0x06, 0x09, 0x09, 0x07, 0x01, 0x09, 0x06, 0x00}, 4},
	{'7', {0x02, 0x02, 0x02, 0x04, 0x08, 0x08, 0x0f, 0x00}, 4},
	{'8', {0x06, 0x09, 0x09, 0x06, 0x09, 0x09, 0x06, 0x00}, 4},
	{'9', {0x06, 0x09, 0x08, 0x0e, 0x09, 0x09, 0x06, 0x00}, 4},

	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0}, // padding
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},

	{'a', {0x00, 0x0e, 0x09, 0x0f, 0x08, 0x06, 0x00, 0x00}, 4},
	{'b', {0x00, 0x06, 0x09, 0x09, 0x07, 0x01, 0x01, 0x00}, 4},
	{'c', {0x00, 0x0e, 0x01, 0x01, 0x01, 0x0e, 0x00, 0x00}, 4},
	{'d', {0x00, 0x0e, 0x09, 0x09, 0x0e, 0x08, 0x08, 0x00}, 4},
	{'e', {0x00, 0x0e, 0x01, 0x0f, 0x09, 0x06, 0x00, 0x00}, 4},
	{'f', {0x01, 0x01, 0x01, 0x07, 0x01, 0x06, 0x00, 0x00}, 3},
	{'g', {0x00, 0x06, 0x09, 0x0d, 0x01, 0x06, 0x00, 0x00}, 4},
	{'h', {0x00, 0x09, 0x09, 0x07, 0x01, 0x01, 0x00, 0x00}, 4},
	{'i', {0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00}, 2},
	{'j', {0x03, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00}, 3},
	{'k', {0x00, 0x05, 0x03, 0x03, 0x05, 0x01, 0x01, 0x00}, 4},
	{'l', {0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00}, 1},
	{'m', {0x00, 0x15, 0x15, 0x15, 0x15, 0x0f, 0x00, 0x00}, 5},
	{'n', {0x00, 0x09, 0x09, 0x09, 0x09, 0x07, 0x00, 0x00}, 4},
	{'o', {0x00, 0x06, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00}, 4},
	{'p', {0x01, 0x01, 0x07, 0x09, 0x09, 0x07, 0x00, 0x00}, 4},
	{'q', {0x08, 0x08, 0x0e, 0x09, 0x09, 0x0e, 0x00, 0x00}, 4},
	{'r', {0x00, 0x01, 0x01, 0x01, 0x09, 0x06, 0x00, 0x00}, 3},
	{'s', {0x00, 0x07, 0x08, 0x06, 0x01, 0x0e, 0x00, 0x00}, 4},
	{'t', {0x00, 0x04, 0x02, 0x02, 0x02, 0x07, 0x02, 0x00}, 3},
	{'u', {0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x00, 0x00}, 4},
	{'v', {0x00, 0x04, 0x0a, 0x11, 0x11, 0x11, 0x00, 0x00}, 5},
	{'w', {0x00, 0x0a, 0x15, 0x15, 0x11, 0x11, 0x00, 0x00}, 5},
	{'x', {0x00, 0x09, 0x09, 0x06, 0x09, 0x09, 0x00, 0x00}, 4},
	{'y', {0x00, 0x02, 0x02, 0x02, 0x05, 0x05, 0x00, 0x00}, 3},
	{'z', {0x00, 0x07, 0x01, 0x02, 0x04, 0x07, 0x00, 0x00}, 4},

	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
	{255, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},

	{'?', {0x02, 0x00, 0x02, 0x06, 0x08, 0x09, 0x06, 0x00}, 4},
};

const display_symbol_t *symbols_lookup_table[sizeof(symbols) / sizeof(symbols[0])] = {0};

static void init_symbols_lookup_table()
{
	for (int i = 0; i < sizeof(symbols) / sizeof(symbols[0]); ++i) {
		const int index = symbols[i].symbol - ASCII_OFFSET;
		symbols_lookup_table[index] = &symbols[i];
	}
}

int gtp_display_init()
{
	init_symbols_lookup_table();

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.", display_dev->name);
		return -1;
	}

	display_get_capabilities(display_dev, &capabilities);
	buf_desc.buf_size = sizeof(buf);
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = capabilities.y_resolution;
	return 0;
}

void gtp_display_clear()
{
	gtp_display_print_sentence("", 0);
}

static inline void set_min_max_display_area(const int min, const int max)
{
	min_x_display_area = min;
	max_x_display_area = max;
}

void gtp_display_set_min_max_display_area(const int min, const int max)
{
	k_mutex_lock(&gtp_display_mutex, K_FOREVER);
	set_min_max_display_area(min, max);
	k_mutex_unlock(&gtp_display_mutex);
}

void gtp_display_print_sentence(const char *s, const size_t size)
{
	__ASSERT_NO_MSG(size < sizeof(sentence));
	k_mutex_lock(&gtp_display_mutex, K_FOREVER);
	strlcpy(sentence, s, sizeof(sentence));
	k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_NEW_WORD);
	k_mutex_unlock(&gtp_display_mutex);
}

static inline void get_display_indexes_to_consider(const int idx, int ditc[2])
{
#define LEFT    0
#define RIGHT   1
#define INVALID -1

	// set default values
	ditc[LEFT] = INVALID;
	ditc[RIGHT] = INVALID;

	if (idx > -8) {
		if (idx < 0) {
			ditc[RIGHT] = 0;

		} else if (idx == 0) {
			ditc[LEFT] = 0;

		} else if (idx < 8) {
			ditc[LEFT] = 0;
			ditc[RIGHT] = 1;

		} else if (idx == 8) {
			ditc[LEFT] = 1;

		} else if (idx < 16) {
			ditc[LEFT] = 1;
			ditc[RIGHT] = 2;

		} else if (idx == 16) {
			ditc[LEFT] = 2;

		} else if (idx < 24) {
			ditc[LEFT] = 2;
			ditc[RIGHT] = 3;

		} else if (idx < 32) {
			ditc[LEFT] = 3;
			ditc[RIGHT] = INVALID;
		}
	}

#undef LEFT
#undef RIGHT
#undef INVALID
}

void gtp_display_set_menu_mode(const bool on)
{
	k_mutex_lock(&gtp_display_mutex, K_FOREVER);

	if (menu_mode == false && on == true) {
		LOG_INF("switching to menu mode");
		menu_mode = on;
		k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_ON);

	} else if (menu_mode == true && on == false) {
		LOG_INF("switching to normal mode");
		menu_mode = on;
		k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_OFF);
	}

	k_mutex_unlock(&gtp_display_mutex);
}

void gtp_display_print_buf(const char *buf)
{
	display_write(display_dev, 0, 0, &buf_desc, buf);
}

static void gtp_display_add_letter(const char mask[8], int *idx, const int max_idx)
{
	if (*idx >= max_idx) {
		return;
	}

	/* The idx represent the horizontal index where the symbol
	 * is positioned.
	 * Ex: an 'A' at idx 0 will be left aligned on the screen
	 *  |A        |
	 * an 'B' at index 2 will be left aligned with 2 columns
	 * of dots of padding.
	 * |  B       |
	 *
	 * As all of our symbols are 8 dot long, we don't consider
	 * ant index <= 8 (nothing is displayed in that case)
	 *
	 * However, we may have an index of -4. half of the symbol
	 * will not be displayed and half of it yes. Example of
	 * a letter 'A' with index eqa=ual to -4
	 *
	 *  Out of screen
	 *               ___________________________
	 *              |                           |
	 *             /|\                          |
	 *            / | \        screen           |
	 *           /  |  \                        |
	 *          /---|---\                       |
	 *         /    |    \                      |
	 *        /     |     \                     |
	 *              |___________________________|
	 *
	 */

	int ditc[2]; // display indexes to consider = ditc
	get_display_indexes_to_consider(*idx, ditc);

	for (int row = 0; row < 8; ++row) {

		if (ditc[0] != -1) {
			const int buf_idx = row + ditc[0] * 8;
			const int shift_by = abs((*idx) % 8);
			// LOG_DBG("at buf[%d] shift left by: %d", buf_idx, shift_by);

			const int x_min = ditc[0] * 8;
			const int x_max = x_min + 8;
			uint8_t content = mask[row] << shift_by;
			uint8_t avoid_cursor_overide_mask = 0;

			if (x_min <= max_idx && max_idx < x_max) {
				avoid_cursor_overide_mask = 0xFF >> (x_max - max_idx);
				content &= avoid_cursor_overide_mask;
			}

			buf[buf_idx] |= content;
		}

		if (ditc[1] != -1 && *idx + 1 < DISPLAY_WIDTH) {
			const int buf_idx = row + ditc[1] * 8;
			const int shift_by = abs(8 - (*idx) % 8) % 8;
			const int x_min = ditc[1] * 8;
			const int x_max = x_min + 8;
			uint8_t content = mask[row] >> shift_by;
			uint8_t avoid_cursor_overide_mask = 0;

			if (x_min <= max_idx && max_idx < x_max) {
				avoid_cursor_overide_mask = 0xFF >> (x_max - max_idx);
				content &= avoid_cursor_overide_mask;
			}

			buf[buf_idx] |= content;
		}
	}
}

static inline int get_total_dot_led_length(const char *s)
{
	int res = 0;
	for (int i = 0; i < strlen(s); ++i) {
		res += symbols_lookup_table[s[i] - ASCII_OFFSET]->width;
		res += 1; // add space between all letters
	}
	return res;
}

static inline void add_menu_mode_arrows(const bool menu_mode)
{
	if (menu_mode) {
		int tmp_idx = 24;
		gtp_display_add_letter(up_arrow_mask, &tmp_idx, DISPLAY_WIDTH);
		tmp_idx = 24;
		gtp_display_add_letter(down_arrow_mask, &tmp_idx, DISPLAY_WIDTH);
	}
}

static void add_sentence(const int global_offset, const char *local_sentence,
			 const int max_x_display)
{
	memset(buf, 0, sizeof(buf));
	int idx = global_offset;

	for (const char *c = local_sentence; *c != '\0'; ++c) {
		if (*c < ASCII_OFFSET ||
		    *c >= (ASCII_OFFSET + sizeof(symbols) / sizeof(symbols[0]))) {
			LOG_WRN("Character '%c' not valid", *c);
			continue;
		}

		const display_symbol_t *symbol = symbols_lookup_table[*c - ASCII_OFFSET];
		if (symbol) {
			gtp_display_add_letter(symbol->mask, &idx, max_x_display);
			idx += symbol->width;
			idx += 1; // space between symbol
		} else {
			LOG_ERR("Character not found in lookup table: %c", *c);
		}
	}
}

static void gtp_display_entry_point(void *, void *, void *)
{
	// init local variables
#define RESET_VARIABLES()                                                                          \
	global_offset = 0;                                                                         \
	total_dot_led_length = get_total_dot_led_length(local_sentence);                           \
	idx = 0;

	char local_sentence[SENTENCE_SIZE] = {0};
	int global_offset = 0;
	int total_dot_led_length = 0;
	int idx = 0;
	int local_max_x_display = max_x_display_area;
	bool local_menu_mode = false;
	bool local_shift_needed = false;

#define INTERRUPTIBLE_SLEEP(max_ms)                                                                \
	for (int i = 0; i < (max_ms / 10); ++i) {                                                  \
		k_msleep(10);                                                                      \
		uint32_t now_event = k_event_wait(&gtp_display_event, GTP_DISPLAY_ALL_EVENTS_MASK, \
						  false, K_NO_WAIT);                               \
		if (now_event != 0 && now_event != event) {                                        \
			break;                                                                     \
		}                                                                                  \
	}

	while (1) {

		uint32_t event = k_event_wait(&gtp_display_event, GTP_DISPLAY_ALL_EVENTS_MASK,
					      false, K_FOREVER);
		// LOG_DBG("event: %d", event);

		if (event & GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_ON) {
			LOG_INF("menu mode on");
			gtp_display_set_min_max_display_area(0, DISPLAY_WIDTH_MENU_ON);
			k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_NEW_WORD);
			k_event_clear(&gtp_display_event, GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_ON);

		} else if (event & GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_OFF) {
			LOG_INF("menu mode off");
			gtp_display_set_min_max_display_area(0, DISPLAY_WIDTH);
			k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_NEW_WORD);
			k_event_clear(&gtp_display_event, GTP_DISPLAY_EVENT_SWITCH_MENU_MODE_OFF);

		} else if (event & GTP_DISPLAY_EVENT_NEW_WORD) {
			k_event_clear(&gtp_display_event, GTP_DISPLAY_EVENT_SHIFT_TEXT);
			k_event_clear(&gtp_display_event, GTP_DISPLAY_EVENT_NEW_WORD);

			k_mutex_lock(&gtp_display_mutex, K_FOREVER);
			strlcpy(local_sentence, sentence, sizeof(local_sentence));
			RESET_VARIABLES();
			local_menu_mode = menu_mode;
			local_max_x_display = max_x_display_area;
			local_shift_needed =
				total_dot_led_length > local_max_x_display ? true : false;
			k_mutex_unlock(&gtp_display_mutex);

			add_sentence(global_offset, local_sentence, local_max_x_display);

			add_menu_mode_arrows(local_menu_mode);
			display_write(display_dev, 0, 0, &buf_desc, buf);

			if (local_shift_needed) {
				INTERRUPTIBLE_SLEEP(500);
				k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_SHIFT_TEXT);
			}

		} else if (event & GTP_DISPLAY_EVENT_SHIFT_TEXT) {
			global_offset--;
			INTERRUPTIBLE_SLEEP(50);

			memset(buf, 0, sizeof(buf));
			idx = global_offset;

			for (const char *c = local_sentence; *c != '\0'; ++c) {
				if (*c < ASCII_OFFSET ||
				    *c >= (ASCII_OFFSET + sizeof(symbols) / sizeof(symbols[0]))) {
					LOG_WRN("Character '%c' not valid", *c);
					continue;
				}

				const display_symbol_t *symbol =
					symbols_lookup_table[*c - ASCII_OFFSET];
				if (symbol) {
					// LOG_INF("add char %c at idx %d", *c, idx);
					gtp_display_add_letter(symbol->mask, &idx,
							       local_max_x_display);
					idx += symbol->width;
					idx += 1; // space between symbol
				} else {
					LOG_ERR("Character not found in lookup table: %c", *c);
				}
			}

			// add up and down arrows if in menu mode
			if (local_menu_mode == true) {
				int tmp_idx = 24;
				gtp_display_add_letter(up_arrow_mask, &tmp_idx, DISPLAY_WIDTH);
				tmp_idx = 24;
				gtp_display_add_letter(down_arrow_mask, &tmp_idx, DISPLAY_WIDTH);
			}

			display_write(display_dev, 0, 0, &buf_desc, buf);

			/* detect if end of shifting */
			if (total_dot_led_length + global_offset <= local_max_x_display) {
				INTERRUPTIBLE_SLEEP(1000);
				k_event_clear(&gtp_display_event, GTP_DISPLAY_EVENT_SHIFT_TEXT);
				k_event_post(&gtp_display_event, GTP_DISPLAY_EVENT_NEW_WORD);
			}

		} else {
			LOG_WRN("unknown event: %d", event);
		}
	}
}
