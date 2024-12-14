#include <gtp_menu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gtp_menu, CONFIG_GTPMENU_LOG_LEVEL);

#define MAX_MENU_NUMBER 10

static int8_t menu_index = 0;
static int8_t init_menu_index = 0;
static int8_t init_max_menu_index = 0;

static bool menu_mode_enabled = true;
static on_gtp_menu_event_cb_t on_gtp_menu_event_cb = NULL;

typedef struct {
	const char *menu_name;
	start_game_func_t start_function;
} menu_t;

static menu_t menus[MAX_MENU_NUMBER] = {NULL};

void gtp_menu_init()
{
	menu_index = 0;
	menu_mode_enabled = true;
	init_menu_index = 0;
	init_max_menu_index = 0;
}

void gtp_menu_next()
{
	if (init_max_menu_index <= 0) {
		return;
	}
	menu_index++;
	menu_index %= init_max_menu_index;
	gtp_menu_raise_cb();
}

void gtp_menu_previous()
{
	if (init_max_menu_index <= 0) {
		return;
	}
	menu_index--;
	if (menu_index < 0) {
		menu_index = init_max_menu_index - 1;
	}
	gtp_menu_raise_cb();
}

bool gtp_menu_is_menu_mode()
{
	return menu_mode_enabled;
}

void gtp_menu_set_title(const char *title, start_game_func_t start_func)
{
	if (init_menu_index >= MAX_MENU_NUMBER) {
		LOG_ERR("menu index out of range");
		return;
	}
	menus[init_menu_index].menu_name = title;
	menus[init_menu_index].start_function = start_func;
	init_menu_index++;
	init_max_menu_index++;
}

void gtp_menu_start_current_game()
{
	if (menus[menu_index].start_function != NULL) {
		menus[menu_index].start_function();
	}
}

void gtp_menu_raise_cb()
{
	if (on_gtp_menu_event_cb != NULL) {
		on_gtp_menu_event_cb(menus[menu_index].menu_name);
	}
}

void gtp_menu_set_event_cb(on_gtp_menu_event_cb_t cb)
{
	on_gtp_menu_event_cb = cb;
}
