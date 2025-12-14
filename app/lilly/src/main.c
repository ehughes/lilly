/*
 * Lilly - Christmas Ornament Photo Slideshow
 *
 * Displays a rotating slideshow of photos on a round 240x240 GC9A01 display.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "images.h"

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

/* LED for status indication */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* Slideshow timing */
#define SLIDE_DURATION_MS 3000

/* Byte-swap macro for big-endian display */
#define SWAP16(x) (((x) >> 8) | (((x) & 0xFF) << 8))

/* RGB565 color definitions (byte-swapped for big-endian display) */
#define RGB565_RED     SWAP16(0xF800)  /* R=31, G=0,  B=0  */
#define RGB565_GREEN   SWAP16(0x07E0)  /* R=0,  G=63, B=0  */
#define RGB565_BLUE    SWAP16(0x001F)  /* R=0,  G=0,  B=31 */
#define RGB565_WHITE   SWAP16(0xFFFF)
#define RGB565_BLACK   SWAP16(0x0000)
#define RGB565_YELLOW  SWAP16(0xFFE0)  /* R=31, G=63, B=0  */
#define RGB565_CYAN    SWAP16(0x07FF)  /* R=0,  G=63, B=31 */
#define RGB565_MAGENTA SWAP16(0xF81F)  /* R=31, G=0,  B=31 */

/* Test pattern buffer - one row at a time to save RAM */
static uint16_t row_buf[240];

static void display_color_test(const struct device *dev)
{
	struct display_buffer_descriptor desc;
	int y;

	desc.buf_size = sizeof(row_buf);
	desc.pitch = 240;
	desc.width = 240;
	desc.height = 1;
	desc.frame_incomplete = false;

	LOG_INF("=== COLOR TEST PATTERN ===");
	LOG_INF("Rows 0-39:    Should be RED");
	LOG_INF("Rows 40-79:   Should be GREEN");
	LOG_INF("Rows 80-119:  Should be BLUE");
	LOG_INF("Rows 120-159: Should be WHITE");
	LOG_INF("Rows 160-199: Should be YELLOW");
	LOG_INF("Rows 200-239: Should be CYAN");

	for (y = 0; y < 240; y++) {
		uint16_t color;

		if (y < 40) {
			color = RGB565_RED;
		} else if (y < 80) {
			color = RGB565_GREEN;
		} else if (y < 120) {
			color = RGB565_BLUE;
		} else if (y < 160) {
			color = RGB565_WHITE;
		} else if (y < 200) {
			color = RGB565_YELLOW;
		} else {
			color = RGB565_CYAN;
		}

		/* Fill row with color */
		for (int x = 0; x < 240; x++) {
			row_buf[x] = color;
		}

		display_write(dev, 0, y, &desc, row_buf);
	}

	LOG_INF("=== What colors do you see? ===");
}

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

	/* Enable USB for picotool and console */
	if (usb_enable(NULL)) {
		LOG_WRN("USB enable failed");
	}

	/* Brief delay for USB enumeration */
	k_sleep(K_MSEC(500));

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

	// /* === COLOR DEBUG MODE === */
	// display_color_test(display_dev);
	// LOG_INF("Color test displayed. Check the screen!");

	// while (1) {
	// 	if (gpio_is_ready_dt(&led)) {
	// 		gpio_pin_toggle_dt(&led);
	// 	}
	// 	k_sleep(K_MSEC(500));
	// }

 /* Slideshow disabled for color debugging */
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
