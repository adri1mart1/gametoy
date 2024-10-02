
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <gtp_buttons.h>

int main()
{
    LOG_INF("Starting");
    gtp_buttons_init();

    while (1) {
        k_msleep(1000);
    }
}
