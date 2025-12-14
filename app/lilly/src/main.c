/*
 * Lilly - Christmas Ornament Photo Slideshow
 *
 * Displays a rotating slideshow of photos on a round 240x240 GC9A01 display.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "images.h"

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

/* LED for status indication */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Slideshow timing */
#define SLIDE_DURATION_MS 3000

static int display_image(const struct device *dev, const image_t *img)
{
	struct display_buffer_descriptor desc;
	int ret;

	desc.buf_size = img->size * sizeof(uint16_t);
	desc.pitch = img->width;
	desc.width = img->width;
	desc.height = img->height;
	desc.frame_incomplete = false;

	ret = display_write(dev, 0, 0, &desc, img->data);
	if (ret < 0) {
		LOG_ERR("Failed to write image %s: %d", img->name, ret);
	}

	return ret;
}

int main(void)
{
	const struct device *display_dev;
	struct display_capabilities caps;
	int current_image = 0;
	int ret;

	LOG_INF("=================================");
	LOG_INF("Lilly - Christmas Ornament");
	LOG_INF("Photo Slideshow");
	LOG_INF("=================================");
	LOG_INF("Images: %d", IMAGE_COUNT);

	/* Initialize LED for status */
	if (gpio_is_ready_dt(&led)) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(&led, 1);
	}

	/* Get display device */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready!");
		while (1) {
			if (gpio_is_ready_dt(&led)) {
				gpio_pin_toggle_dt(&led);
			}
			k_sleep(K_MSEC(100));
		}
	}

	/* Get display capabilities */
	display_get_capabilities(display_dev, &caps);
	LOG_INF("Display: %s", display_dev->name);
	LOG_INF("Resolution: %dx%d", caps.x_resolution, caps.y_resolution);

	/* Turn on display */
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_WRN("Failed to turn off blanking: %d", ret);
	}

	LOG_INF("Starting slideshow (%d ms per image)", SLIDE_DURATION_MS);

	/* Main slideshow loop */
	while (1) {
		const image_t *img = &images[current_image];

		LOG_INF("Displaying: %s (%d/%d)", img->name, current_image + 1, IMAGE_COUNT);

		ret = display_image(display_dev, img);
		if (ret < 0) {
			LOG_ERR("Display failed, retrying...");
			k_sleep(K_MSEC(100));
			continue;
		}

		/* Toggle LED to show activity */
		if (gpio_is_ready_dt(&led)) {
			gpio_pin_toggle_dt(&led);
		}

		/* Wait before next image */
		k_sleep(K_MSEC(SLIDE_DURATION_MS));

		/* Next image (wrap around) */
		current_image = (current_image + 1) % IMAGE_COUNT;
	}

	return 0;
}
