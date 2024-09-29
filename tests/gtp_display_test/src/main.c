#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

int main() {

    while (1) {
        LOG_INF("Works");
        k_msleep(1000);
    }
    return 0;
}
