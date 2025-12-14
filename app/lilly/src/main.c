#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

int main(void)
{
    const struct device *display_dev;
    struct display_capabilities caps;

    LOG_INF("Lilly - Christmas Ornament Slideshow");
    LOG_INF("Starting...");

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return 0;
    }

    display_get_capabilities(display_dev, &caps);
    LOG_INF("Display: %dx%d", caps.x_resolution, caps.y_resolution);

    display_blanking_off(display_dev);

    /* For now, just a simple test pattern */
    LOG_INF("Display initialized successfully!");

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
