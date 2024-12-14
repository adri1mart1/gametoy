
#ifndef GTP_MENU_H__
#define GTP_MENU_H__

#include <zephyr/types.h>
#include <stdbool.h>

void gtp_menu_init();
void gtp_menu_next();
void gtp_menu_previous();
bool gtp_menu_is_menu_mode();
typedef void (*start_game_func_t)();
void gtp_menu_set_title(const char *title, start_game_func_t start_func);
void gtp_menu_start_current_game();
void gtp_menu_raise_cb();
typedef void (*on_gtp_menu_event_cb_t)(const char *menu_to_display);
void gtp_menu_set_event_cb(on_gtp_menu_event_cb_t cb);

#endif // GTP_MENU_H__
