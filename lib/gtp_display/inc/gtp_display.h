#ifndef GTP_DISPLAY_H__
#define GTP_DISPLAY_H__

#include <zephyr/types.h>
#include <stdbool.h>

int gtp_display_init();
void gtp_display_clear();
void gtp_display_set_min_max_display_area(const int min, const int max);
void gtp_display_print_sentence(const char *s, const size_t size);
void gtp_display_set_menu_mode(const bool on);

#endif // GTP_DISPLAY_H__
