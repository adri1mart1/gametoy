#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "gtp_display.h"

int main()
{
	gtp_display_init();

	int i = 0;

	const char *s1 = "helloworld";
	gtp_display_print_sentence(s1, strlen(s1));

	k_msleep(3000);

	gtp_display_set_menu_mode(true);

	k_msleep(10000);
	gtp_display_set_next_menu();

	/*
		k_msleep(10000);
		gtp_display_set_next_menu();

		k_msleep(10000);
		gtp_display_set_next_menu();

		k_msleep(10000);
		gtp_display_set_next_menu();
	*/

#if 0
	while (1) {
		k_msleep(500);

#if 1
		const char *s3 = "0123456789";
		gtp_display_print_sentence(s3, strlen(s3));
		continue;

#endif
		const char *s = "this is my super sentence with spaces";

		if (i == 3) {
			gtp_display_print_sentence(s, strlen(s));
		}

#if 1
		const char *s2 = "now printing numbers 0 1 2 3 4 5 6 7 8 9";
		if (i == 20) {
			gtp_display_print_sentence(s2, strlen(s2));
		}
#endif
		i++;
	}
#endif
	return 0;
}
