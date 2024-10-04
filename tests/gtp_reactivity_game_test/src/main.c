#include <gtp_reactivity_game.h>
#include <gtp_buttons.h>
#include <zephyr/kernel.h>

int main()
{
	gtp_buttons_init();
	gtp_reactivity_game_start();

	while (1) {
		k_msleep(1000);
	}
}
