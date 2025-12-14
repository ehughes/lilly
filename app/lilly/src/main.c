/*
 * Lilly - Christmas Ornament Photo Slideshow
 *
 * Display test application based on Zephyr display sample.
 * Draws colored rectangles in corners with animated greyscale.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

/* LED for status indication */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Color definitions for RGB565 */
#define RGB565_RED    0xF800
#define RGB565_GREEN  0x07E0
#define RGB565_BLUE   0x001F
#define RGB565_WHITE  0xFFFF
#define RGB565_BLACK  0x0000

static void fill_buffer_rgb565(uint16_t color, uint8_t *buf, size_t buf_size)
{
	for (size_t i = 0; i < buf_size; i += 2) {
		*(uint16_t *)(buf + i) = color;
	}
}

int main(void)
{
	const struct device *display_dev;
	struct display_capabilities caps;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size;
	uint8_t *buf;
	uint16_t colors[] = {RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_WHITE};
	int color_idx = 0;
	int ret;

	LOG_INF("=================================");
	LOG_INF("Lilly - Christmas Ornament");
	LOG_INF("Display Test Application");
	LOG_INF("=================================");

	/* Initialize LED for status */
	if (gpio_is_ready_dt(&led)) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(&led, 1);
	}

	/* Get display device */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready!");
		LOG_ERR("Check SPI wiring and overlay configuration");
		while (1) {
			if (gpio_is_ready_dt(&led)) {
				gpio_pin_toggle_dt(&led);
			}
			k_sleep(K_MSEC(100)); /* Fast blink = error */
		}
	}

	/* Get display capabilities */
	display_get_capabilities(display_dev, &caps);
	LOG_INF("Display: %s", display_dev->name);
	LOG_INF("Resolution: %dx%d", caps.x_resolution, caps.y_resolution);
	LOG_INF("Pixel format: %d", caps.current_pixel_format);

	/* Allocate buffer for a horizontal stripe */
	buf_size = caps.x_resolution * 10 * 2; /* 10 rows, RGB565 */
	buf = k_malloc(buf_size);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate display buffer");
		return 0;
	}

	/* Turn on display */
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_WRN("Failed to turn off blanking: %d", ret);
	}

	LOG_INF("Starting display test - cycling colors");
	LOG_INF("Watch for SPI signals on GPIO 10 (CLK), 11 (MOSI)");

	/* Main loop - fill screen with cycling colors */
	while (1) {
		uint16_t color = colors[color_idx];

		LOG_INF("Filling screen with color 0x%04X", color);

		/* Fill buffer with current color */
		fill_buffer_rgb565(color, buf, buf_size);

		/* Write color to entire screen in stripes */
		buf_desc.buf_size = buf_size;
		buf_desc.pitch = caps.x_resolution;
		buf_desc.width = caps.x_resolution;
		buf_desc.height = 10;
		buf_desc.frame_incomplete = false;

		for (int y = 0; y < caps.y_resolution; y += 10) {
			if ((caps.y_resolution - y) < 10) {
				buf_desc.height = caps.y_resolution - y;
			}
			ret = display_write(display_dev, 0, y, &buf_desc, buf);
			if (ret < 0) {
				LOG_ERR("Display write failed at y=%d: %d", y, ret);
				break;
			}
		}

		/* Toggle LED to show activity */
		if (gpio_is_ready_dt(&led)) {
			gpio_pin_toggle_dt(&led);
		}

		/* Next color */
		color_idx = (color_idx + 1) % ARRAY_SIZE(colors);

		k_sleep(K_SECONDS(2));
	}

	return 0;
}
