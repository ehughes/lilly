# Lilly - Christmas Ornament Photo Slideshow

A memorial photo slideshow ornament using the Waveshare RP2350-LCD-1.28 board with a 240x240 round GC9A01 display.

## Hardware

- **Board**: Waveshare RP2350-LCD-1.28
- **MCU**: RP2350 (Dual Cortex-M33 @ 150MHz)
- **Display**: 1.28" Round IPS LCD, 240x240, GC9A01A controller
- **IMU**: QMI8658 (accelerometer + gyroscope)

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
