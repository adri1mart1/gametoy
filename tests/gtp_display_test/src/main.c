#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include "gtp_display.h"

int main()
{

	gtp_display_init();

	// gtp_display_dump_alphabet();

	while (1) {
		k_msleep(1000);
	}
	return 0;
}
