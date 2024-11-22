#include <gtp_menu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_menu, CONFIG_GTPMENU_LOG_LEVEL);

#define MAX_GAME_NAME_LENGTH 16

typedef struct {
	char game_name[MAX_GAME_NAME_LENGTH];
} menu_t;

static int menu_index = 0;
static bool menu_mode_enabled = true;
static on_gtp_menu_event_cb_t on_gtp_menu_event_cb = NULL;

static menu_t menus[] = {{"abc"}, {"abcd"}, {"abcde"}, {"hello this ismy"}};

void gtp_menu_init()
{
	menu_index = 0;
	menu_mode_enabled = true;
}

void gtp_menu_next()
{
	menu_index++;
	menu_index %= (sizeof(menus) / sizeof(menus[0]));
	gtp_menu_raise_cb();
}

void gtp_menu_previous()
{
	menu_index--;
	if (menu_index < 0) {
		menu_index = sizeof(menus) / sizeof(menus[0]) - 1;
	}
	gtp_menu_raise_cb();
}

bool gtp_menu_is_menu_mode()
{
	return menu_mode_enabled;
}

void gtp_menu_raise_cb()
{
	if (on_gtp_menu_event_cb != NULL) {
		on_gtp_menu_event_cb(menus[menu_index].game_name);
	}
}

void gtp_menu_set_event_cb(on_gtp_menu_event_cb_t cb)
{
	on_gtp_menu_event_cb = cb;
}
