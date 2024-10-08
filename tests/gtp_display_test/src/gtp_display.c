#include "gtp_display.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtpdisplay, LOG_LEVEL_DBG);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static struct display_capabilities capabilities;
static struct display_buffer_descriptor buf_desc;

#define STACK_SIZE 1024
#define PRIORITY   5

#define DISPLAY_WIDTH 32
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

typedef struct {
	char symbol;
	char mask[8];
	int width;
} display_symbol_t;

#define ASCII_OFFSET  'a'
#define ALPHABET_SIZE 26

const display_symbol_t letters[] = {
	{'a', {0x00, 0x0e, 0x09, 0x0f, 0x08, 0x06, 0x00, 0x00}, 4}, // a
	{'b', {0x00, 0x06, 0x09, 0x09, 0x07, 0x01, 0x01, 0x00}, 4}, // b
	{'c', {0x00, 0x0e, 0x01, 0x01, 0x01, 0x0e, 0x00, 0x00}, 4}, // c
	{'d', {0x00, 0x0e, 0x09, 0x09, 0x0e, 0x08, 0x08, 0x00}, 4}, // d
	{'e', {0x00, 0x0e, 0x01, 0x0f, 0x09, 0x06, 0x00, 0x00}, 4}, // e
	{'f', {0x01, 0x01, 0x01, 0x07, 0x01, 0x06, 0x00, 0x00}, 3}, // f
	{'g', {0x00, 0x06, 0x09, 0x0d, 0x01, 0x06, 0x00, 0x00}, 4}, // g
	{'h', {0x00, 0x09, 0x09, 0x07, 0x01, 0x01, 0x00, 0x00}, 4}, // h
	{'i', {0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00}, 2}, // i
	{'j', {0x03, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00}, 3}, // j
	{'k', {0x00, 0x05, 0x03, 0x03, 0x05, 0x01, 0x01, 0x00}, 4}, // k
	{'l', {0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00}, 1}, // l
	{'m', {0x00, 0x15, 0x15, 0x15, 0x15, 0x0f, 0x00, 0x00}, 5}, // m
	{'n', {0x00, 0x09, 0x09, 0x09, 0x09, 0x07, 0x00, 0x00}, 4}, // n
	{'o', {0x00, 0x06, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00}, 4}, // o
	{'p', {0x01, 0x01, 0x07, 0x09, 0x09, 0x07, 0x00, 0x00}, 4}, // p
	{'q', {0x08, 0x08, 0x0e, 0x09, 0x09, 0x0e, 0x00, 0x00}, 4}, // q
	{'r', {0x00, 0x01, 0x01, 0x01, 0x09, 0x06, 0x00, 0x00}, 3}, // r
	{'s', {0x00, 0x07, 0x08, 0x06, 0x01, 0x0e, 0x00, 0x00}, 4}, // s
	{'t', {0x00, 0x04, 0x02, 0x02, 0x02, 0x07, 0x02, 0x00}, 3}, // t
	{'u', {0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x00, 0x00}, 4}, // u
	{'v', {0x00, 0x04, 0x0a, 0x11, 0x11, 0x11, 0x00, 0x00}, 5}, // v
	{'w', {0x00, 0x0a, 0x15, 0x15, 0x11, 0x11, 0x00, 0x00}, 5}, // w
	{'x', {0x00, 0x09, 0x09, 0x06, 0x09, 0x09, 0x00, 0x00}, 4}, // x
	{'y', {0x00, 0x02, 0x02, 0x02, 0x05, 0x05, 0x00, 0x00}, 3}, // y
	{'z', {0x00, 0x07, 0x01, 0x02, 0x04, 0x07, 0x00, 0x00}, 4}, // z
};

#define DIGIT_OFFSET '0'
#define DIGIT_SIZE   10

const display_symbol_t digits[] = {
	{'0', {0x1c, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c}, 4},
	{'1', {0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0c, 0x08}, 4},
	{'2', {0x3e, 0x04, 0x08, 0x10, 0x20, 0x20, 0x22, 0x1c}, 4},
	{'3', {0x1c, 0x22, 0x20, 0x20, 0x18, 0x20, 0x22, 0x1c}, 4},
	{'4', {0x20, 0x20, 0x3e, 0x22, 0x24, 0x28, 0x30, 0x20}, 4},
	{'5', {0x1c, 0x22, 0x20, 0x20, 0x1e, 0x02, 0x02, 0x3e}, 4},
	{'6', {0x1c, 0x22, 0x22, 0x22, 0x1e, 0x02, 0x22, 0x1c}, 4},
	{'7', {0x04, 0x04, 0x04, 0x08, 0x10, 0x20, 0x20, 0x3e}, 4},
	{'8', {0x1c, 0x22, 0x22, 0x22, 0x1c, 0x22, 0x22, 0x1c}, 4},
	{'9', {0x1c, 0x22, 0x20, 0x3c, 0x22, 0x22, 0x22, 0x1c}, 4},
};

const display_symbol_t *letters_lookup_table[ALPHABET_SIZE] = {0};
const display_symbol_t *digits_lookup_table[DIGIT_SIZE] = {0};

static void init_letters_lookup_table()
{
	for (int i = 0; i < sizeof(letters) / sizeof(letters[0]); ++i) {
		const int index = letters[i].symbol - ASCII_OFFSET;
		letters_lookup_table[index] = &letters[i];
	}
}

void gtp_display_word(const char *word);

int gtp_display_init()
{
	init_letters_lookup_table();

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

void gtp_display_dump_alphabet()
{
	const char *test = "h";
	LOG_DBG("gtp displaying: %s", test);
	gtp_display_word(test);
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

static void gtp_display_add_letter(const char mask[8], int *idx)
{
	if (*idx >= DISPLAY_WIDTH) {
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
	 * */

	// display indexes to consider = ditc
	int ditc[2];
	get_display_indexes_to_consider(*idx, ditc);
	// LOG_INF("idx: %d ditc: %d %d", *idx, ditc[0], ditc[1]);

	for (int row = 0; row < 8; ++row) {

		if (ditc[0] != -1) {
			const int buf_idx = row + ditc[0] * 8;
			const int shift_by = abs((*idx) % 8);
			// LOG_DBG("at buf[%d] shift left by: %d", buf_idx, shift_by);
			buf[buf_idx] |= (mask[row] << shift_by);
		}
		if (ditc[1] != -1 && *idx + 1 < DISPLAY_WIDTH) {
			const int buf_idx = row + ditc[1] * 8;
			const int shift_by = abs(8 - (*idx) % 8);

			// LOG_DBG("at buf[%d] shift right by: %d", buf_idx, shift_by);
			buf[buf_idx] |= (mask[row] >> shift_by);
		}
	}

#if 0
	for (int i = 0; i < 2; ++i) {
		const int display_idx = ditc[i];
		LOG_DBG("display_idx: %d", display_idx);

		if (display_idx == -1) {
			continue;
		}


		// apply letter mask
		for (int j = 0; j < 8; ++j) {
			const int row = display_idx * 8 + j;
			int content;
			if (i == 0) {
				const int shift_by = (*idx) % 8;
				LOG_DBG("1 shift_by: %d", shift_by);
				content = mask[j] << shift_by;
			} else {
				const int shift_by = 8 - (*idx) % 8;
				LOG_DBG("2 shift_by: %d", shift_by);
				content = mask[j] >> shift_by;
			}

			buf[row] = buf[row] | content;
		}
	}
#endif
}

void gtp_display_word(const char *word)
{
	int idx = -2;

	memset(buf, 0, sizeof(buf));
	LOG_INF("idx: %d", idx);
	for (const char *c = word; *c != '\0'; ++c) {
		if (*c < ASCII_OFFSET || *c >= (ASCII_OFFSET + ALPHABET_SIZE)) {
			LOG_WRN("Character %c out of bounds", *c);
			continue;
		}

		const display_symbol_t *letter = letters_lookup_table[*c - ASCII_OFFSET];
		if (letter) {
			LOG_DBG("display letter %c at idx: %d", letter->symbol, idx);
			gtp_display_add_letter(letter->mask, &idx);
			idx += letter->width;
			idx += 1; // space between letters
		} else {
			LOG_ERR("Character not found in lookup table: %c", *c);
		}
	}

	display_write(display_dev, 0, 0, &buf_desc, buf);
}

static void gtp_display_entry_point(void *, void *, void *)
{
	k_msleep(1000);

	int global_offset = 0;
	const char *word = "abcdefghijklmnopqrstuvwxyz";

	while (1) {
		int idx = global_offset;

		memset(buf, 0, sizeof(buf));
		LOG_INF("idx: %d", idx);
		for (const char *c = word; *c != '\0'; ++c) {
			if (*c < ASCII_OFFSET || *c >= (ASCII_OFFSET + ALPHABET_SIZE)) {
				LOG_WRN("Character %c out of bounds", *c);
				continue;
			}

			const display_symbol_t *letter = letters_lookup_table[*c - ASCII_OFFSET];
			if (letter) {
				gtp_display_add_letter(letter->mask, &idx);
				idx += letter->width;
				idx += 1; // space between letters
			} else {
				LOG_ERR("Character not found in lookup table: %c", *c);
			}
		}

		display_write(display_dev, 0, 0, &buf_desc, buf);
		global_offset--;

		k_msleep(100);
	}
}
