# Lilly - Christmas Ornament Photo Slideshow

A memorial photo slideshow ornament for Lilly, built using the Waveshare RP2350-LCD-1.28 board with a 240x240 round GC9A01 display.

## Project Goals

- Display a rotating slideshow of photos on a round 1.28" LCD
- Smooth crossfade transitions between images
- Battery-powered for tree mounting
- 3D printed ornament enclosure
- Simple, minimal Zephyr RTOS implementation

## Hardware

| Component | Details |
|-----------|---------|
| **Board** | Waveshare RP2350-Touch-LCD-1.28 |
| **MCU** | RP2350 (Dual Cortex-M33 @ 150MHz) |
| **Flash** | 4MB |
| **RAM** | 520KB SRAM |
| **Display** | 1.28" Round IPS LCD, 240x240, GC9A01A controller, SPI |
| **IMU** | QMI8658 (accelerometer + gyroscope) |
| **Battery** | 3.7V LiPo via MX1.25 header (ETA6096 charge IC) |

### Pin Mapping (Waveshare RP2350-Touch-LCD-1.28)

| Function | GPIO | Notes |
|----------|------|-------|
| SPI1_CLK | 10 | Display clock |
| SPI1_MOSI | 11 | Display data |
| LCD_CS | 9 | Chip select |
| LCD_DC | 8 | Data/Command |
| LCD_RST | 13 | Reset |
| LCD_BL | 25 | Backlight (PWM) |
| I2C1_SDA | 6 | IMU/Touch data |
| I2C1_SCL | 7 | IMU/Touch clock |
| Touch_INT | 21 | Touch interrupt |
| Touch_RST | 22 | Touch reset |

## Quick Start

### Prerequisites

- Zephyr SDK installed
- `west` tool installed (`pip install west`)

### Initialize Workspace

```bash
# Create and enter workspace directory
mkdir lilly-workspace && cd lilly-workspace

# Initialize from this manifest repo
west init -m https://github.com/ehughes/lilly.git

# Fetch Zephyr and dependencies (uses allow-list for faster clone)
west update

# Set up Zephyr environment
source zephyr/zephyr-env.sh   # Linux/macOS
# or
zephyr\zephyr-env.cmd         # Windows
```

### Build Blinky (Test Setup)

```bash
west build -b rpi_pico2/rp2350a/m33 zephyr/samples/basic/blinky
```

### Build Lilly App

```bash
west build -b rpi_pico2/rp2350a/m33 lilly/app/lilly
```

### Flash

Hold BOOTSEL button, connect USB, release button, then:

```bash
cp build/zephyr/zephyr.uf2 /path/to/RPI-RP2
```

## Project Structure

```
lilly-workspace/
├── lilly/              # This repo (manifest + app)
│   ├── west.yml        # West manifest
│   ├── app/
│   │   └── lilly/      # Main slideshow application
│   └── boards/         # Custom board definitions
├── zephyr/             # Zephyr RTOS
├── modules/
│   ├── hal/
│   │   └── hal_rpi_pico/
│   └── lib/
│       ├── cmsis/
│       └── picolibc/
└── build/              # Build output
```

## License

MIT
