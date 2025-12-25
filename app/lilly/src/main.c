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
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
#include <string.h>

/* For USB bootloader reboot */
#include <pico/bootrom.h>

#include "images.h"

LOG_MODULE_REGISTER(lilly, LOG_LEVEL_INF);

/* USB Device Next setup */
#define USB_VID 0x2E8A  /* Raspberry Pi */
#define USB_PID 0x000A

USBD_DEVICE_DEFINE(usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   USB_VID, USB_PID);
USBD_DESC_LANG_DEFINE(usbd_lang);
USBD_DESC_MANUFACTURER_DEFINE(usbd_mfr, "Lilly");
USBD_DESC_PRODUCT_DEFINE(usbd_prod, "Christmas Ornament");
USBD_DESC_CONFIG_DEFINE(usbd_cfg_desc, "Default");
USBD_CONFIGURATION_DEFINE(usbd_config, USB_SCD_SELF_POWERED, 100, &usbd_cfg_desc);

static int usb_init(void)
{
	int ret;

	ret = usbd_add_descriptor(&usbd, &usbd_lang);
	if (ret) {
		return ret;
	}

	ret = usbd_add_descriptor(&usbd, &usbd_mfr);
	if (ret) {
		return ret;
	}

	ret = usbd_add_descriptor(&usbd, &usbd_prod);
	if (ret) {
		return ret;
	}

	ret = usbd_add_configuration(&usbd, USBD_SPEED_FS, &usbd_config);
	if (ret) {
		return ret;
	}

	ret = usbd_register_all_classes(&usbd, USBD_SPEED_FS, 1, NULL);
	if (ret) {
		return ret;
	}

	ret = usbd_init(&usbd);
	if (ret) {
		return ret;
	}

	return usbd_enable(&usbd);
}

/* Backlight GPIO - must be turned on for display to be visible */
static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(DT_NODELABEL(lcd_backlight), gpios);

/* CDC ACM UART for console and bootloader trigger */
static const struct device *cdc_dev;

/*
 * Check for bootloader reboot command ('R' on USB serial)
 * Call this periodically in your main loop
 */
static void check_usb_bootloader_cmd(void)
{
	uint8_t c;

	if (cdc_dev && uart_poll_in(cdc_dev, &c) == 0) {
		if (c == 'R' || c == 'r') {
			LOG_INF("Rebooting to USB bootloader...");
			k_sleep(K_MSEC(100)); /* Let log flush */
			reset_usb_boot(0, 0);
		}
	}
}

/* Slideshow timing */
#define SLIDE_DURATION_MS     6000  

#define CROSSFADE_STEPS       (50)

/* Full framebuffer mode: uses ~112KB RAM but fewer SPI transactions */
#define USE_FULL_FRAMEBUFFER  1

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

#if USE_FULL_FRAMEBUFFER
/* Full framebuffer: 240x240x2 = 115,200 bytes (~112KB) */
static uint16_t frame_buf[IMAGE_WIDTH * IMAGE_HEIGHT];
#else
/* Row buffer for display output */
static uint16_t row_buf[IMAGE_WIDTH];
#endif

/*
 * Convert RGB888 to RGB565 (byte-swapped for big-endian display)
 */
static inline uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
	return SWAP16(rgb565);
}

#if !USE_FULL_FRAMEBUFFER
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
#endif

/*
 * Fade in an image from black
 * alpha: 0 = black, 255 = full image
 */
static int display_fade_from_black(const struct device *dev,
				   const image_t *img,
				   uint8_t alpha)
{
	struct display_buffer_descriptor desc;
	int ret = 0;
	const uint8_t *src = img->data;

#if USE_FULL_FRAMEBUFFER
	uint16_t *dst = frame_buf;
	for (int i = 0; i < IMAGE_PIXELS; i++) {
		uint8_t r = (src[0] * alpha) >> 8;
		uint8_t g = (src[1] * alpha) >> 8;
		uint8_t b = (src[2] * alpha) >> 8;
		*dst++ = rgb888_to_rgb565(r, g, b);
		src += IMAGE_STRIDE;
	}

	desc.buf_size = sizeof(frame_buf);
	desc.pitch = IMAGE_WIDTH;
	desc.width = IMAGE_WIDTH;
	desc.height = IMAGE_HEIGHT;
	desc.frame_incomplete = false;

	ret = display_write(dev, 0, 0, &desc, frame_buf);
#else
	desc.buf_size = sizeof(row_buf);
	desc.pitch = IMAGE_WIDTH;
	desc.width = IMAGE_WIDTH;
	desc.height = 1;
	desc.frame_incomplete = false;

	for (int y = 0; y < IMAGE_HEIGHT; y++) {
		for (int x = 0; x < IMAGE_WIDTH; x++) {
			uint8_t r = (src[0] * alpha) >> 8;
			uint8_t g = (src[1] * alpha) >> 8;
			uint8_t b = (src[2] * alpha) >> 8;

			row_buf[x] = rgb888_to_rgb565(r, g, b);
			src += IMAGE_STRIDE;
		}

		ret = display_write(dev, 0, y, &desc, row_buf);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return ret;
}

/*
 * Display a crossfade blend of two RGB888 images
 * alpha: 0 = show img_from, 255 = show img_to
 * Blending is done in 8-bit RGB space, then converted to RGB565
 */
static int display_crossfade(const struct device *dev,
			     const image_t *img_from,
			     const image_t *img_to,
			     uint8_t alpha)
{
	struct display_buffer_descriptor desc;
	int ret = 0;
	uint8_t inv_alpha = 255 - alpha;
	const uint8_t *src_from = img_from->data;
	const uint8_t *src_to = img_to->data;

#if USE_FULL_FRAMEBUFFER
	uint16_t *dst = frame_buf;
	for (int i = 0; i < IMAGE_PIXELS; i++) {
		uint8_t r = (src_from[0] * inv_alpha + src_to[0] * alpha) >> 8;
		uint8_t g = (src_from[1] * inv_alpha + src_to[1] * alpha) >> 8;
		uint8_t b = (src_from[2] * inv_alpha + src_to[2] * alpha) >> 8;
		*dst++ = rgb888_to_rgb565(r, g, b);
		src_from += IMAGE_STRIDE;
		src_to += IMAGE_STRIDE;
	}

	desc.buf_size = sizeof(frame_buf);
	desc.pitch = IMAGE_WIDTH;
	desc.width = IMAGE_WIDTH;
	desc.height = IMAGE_HEIGHT;
	desc.frame_incomplete = false;

	ret = display_write(dev, 0, 0, &desc, frame_buf);
#else
	desc.buf_size = sizeof(row_buf);
	desc.pitch = IMAGE_WIDTH;
	desc.width = IMAGE_WIDTH;
	desc.height = 1;
	desc.frame_incomplete = false;

	/* Blend and write row by row */
	for (int y = 0; y < IMAGE_HEIGHT; y++) {
		/* Blend each pixel: simple 8-bit math */
		for (int x = 0; x < IMAGE_WIDTH; x++) {
			uint8_t r = (src_from[0] * inv_alpha + src_to[0] * alpha) >> 8;
			uint8_t g = (src_from[1] * inv_alpha + src_to[1] * alpha) >> 8;
			uint8_t b = (src_from[2] * inv_alpha + src_to[2] * alpha) >> 8;

			row_buf[x] = rgb888_to_rgb565(r, g, b);

			src_from += IMAGE_STRIDE;
			src_to += IMAGE_STRIDE;
		}

		ret = display_write(dev, 0, y, &desc, row_buf);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return ret;
}

int main(void)
{
	const struct device *display_dev;
	struct display_capabilities caps;
	int current_image = 0;
	int ret;

	/* Enable USB for console */
	if (usb_init()) {
		LOG_WRN("USB init failed");
	}

	/* Get CDC ACM device for bootloader command */
	cdc_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	if (!device_is_ready(cdc_dev)) {
		LOG_WRN("CDC ACM device not ready");
		cdc_dev = NULL;
	}

	/* Brief delay for USB enumeration */
	k_sleep(K_MSEC(500));
	LOG_INF("Send 'R' over USB serial to reboot to bootloader");

	LOG_INF("=================================");
	LOG_INF("Lilly - Christmas Ornament");
	LOG_INF("Photo Slideshow");
	LOG_INF("=================================");
	LOG_INF("Images: %d", IMAGE_COUNT);

	if (gpio_is_ready_dt(&backlight)) {
		gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(&backlight, 1);
		LOG_INF("Backlight ON (GPIO %d)", backlight.pin);
	} else {
		LOG_ERR("Backlight GPIO not ready!");
	}

	/* Get display device */
	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready!");
		while (1) {
			k_sleep(K_MSEC(1000));
		}
	}

	/* Get display capabilities */
	display_get_capabilities(display_dev, &caps);
	LOG_INF("Display: %s", display_dev->name);
	LOG_INF("Resolution: %dx%d", caps.x_resolution, caps.y_resolution);
#if USE_FULL_FRAMEBUFFER
	LOG_INF("Mode: Full framebuffer (%d KB)", sizeof(frame_buf) / 1024);
#else
	LOG_INF("Mode: Row-by-row (%d bytes)", sizeof(row_buf));
#endif

	/* Turn on display */
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_WRN("Failed to turn off blanking: %d", ret);
	}

	/* Fade in first image from black */
	const image_t *img = &images[current_image];

	for (int step = 0; step <= CROSSFADE_STEPS; step++) {
		uint8_t alpha = (step * 255) / CROSSFADE_STEPS;
		ret = display_fade_from_black(display_dev, img, alpha);
		if (ret < 0) {
			LOG_ERR("Fade-in failed: %d", ret);
			break;
		}
	}

	/* Main slideshow loop */
	while (1) {

		/* Hold on current image */
		for (int i = 0; i < (SLIDE_DURATION_MS) / 100; i++) {
			k_sleep(K_MSEC(100));
		}

		/* Calculate next image index */
		int next_image = (current_image + 1) % IMAGE_COUNT;
		const image_t *img_from = &images[current_image];
		const image_t *img_to = &images[next_image];

		LOG_INF("Crossfading to: %s (%d/%d)", img_to->name, next_image + 1, IMAGE_COUNT);

		/* Crossfade transition */
		for (int step = 0; step <= CROSSFADE_STEPS; step++) {
	
			uint8_t alpha = (step * 255) / CROSSFADE_STEPS;

			ret = display_crossfade(display_dev, img_from, img_to, alpha);

		}

		/* Move to next image */
		current_image = next_image;
	}

	return 0;
}
