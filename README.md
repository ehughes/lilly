# Lilly - Christmas Ornament Photo Slideshow

A memorial photo slideshow ornament for Lilly, built using the Waveshare RP2350-LCD-1.28 board with a 240x240 round display.

https://www.waveshare.com/rp2350-lcd-1.28.htm

### Prerequisites

Zephyr:

https://docs.zephyrproject.org/latest/develop/getting_started/index.html

### Setup

This project is an application as west manifest

```bash
# Create and enter workspace directory
mkdir lilly-workspace && cd lilly-workspace

# Initialize from this manifest repo
west init -m https://github.com/ehughes/lilly.git

# Fetch Zephyr and dependencies (uses allow-list for faster clone)
west update

```

### Build 

cd into the folder app/lilly filder

```bash
west build -b rpi_pico2/rp2350a/m33 
```

We are using the pico2 board with an application dts overlay to make things work on the waveshare board

.uf2 will be in the build/zephyr folder (zephyr.uf2)

### 3D

There is a step model for the display holder in the 3D folder.  Also includes an Alibre Design file.


