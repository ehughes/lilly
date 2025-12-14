#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

/* LED on Pico 2 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    int ret;

    LOG_INF("Lilly - Christmas Ornament Slideshow");
    LOG_INF("Starting...");

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device not ready");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED");
        return 0;
    }

    LOG_INF("Hardware initialized - LED blinking");

    while (1) {
        gpio_pin_toggle_dt(&led);
        k_sleep(K_MSEC(500));
    }

    return 0;
}
