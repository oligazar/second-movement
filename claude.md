# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Second Movement is a work-in-progress refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net). This is an embedded C firmware project for ARM Cortex microcontrollers that drives a custom smartwatch platform with modular "watch faces" (applications).

Key architectural innovation: Uses a custom 32-bit RTC counter (`rtc32.c`) instead of the standard RTC peripheral for improved precision and efficient timing operations.

## Building the Firmware

### Build Commands

Build for hardware (requires BOARD and DISPLAY parameters):
```bash
make BOARD=sensorwatch_pro DISPLAY=classic
make BOARD=sensorwatch_red DISPLAY=classic
make BOARD=sensorwatch_blue DISPLAY=custom
make BOARD=sensorwatch_green DISPLAY=classic
```

Build for emulator (requires emscripten):
```bash
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
# Then visit http://localhost:8000/firmware.html
```

Optional build flags:
- `TIMESET=minute` - Set watch time at build (options: `year`, `day`, `minute`)
- `NOSLEEP=1` - Disable low energy mode (define `MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN`)

### Installation

Install to watch via UF2 bootloader:
```bash
make install  # Copies build/firmware.uf2 to WATCHBOOT drive
```

Or manually copy `build/firmware.uf2` to the watch after entering bootloader mode (double-tap reset button).

### Dependencies

- GNU Arm Embedded Toolchain (`gcc-arm-none-eabi`)
- Git submodules: `git submodule update --init --recursive`
- For emulator: emscripten
- Optional: Nix flake provides complete dev environment

## Architecture

### Movement System (movement.c/movement.h)

Movement is the core application framework that manages watch faces, handles events, and coordinates hardware peripherals. It implements:

- **Watch Face Management**: Array-based rotation through faces defined in `movement_config.h`
- **Event System**: Button presses, ticks, timeouts, and background tasks are dispatched as `movement_event_t` events
- **Power Management**: Automatic transition to low-energy mode based on inactivity intervals
- **RTC Integration**: Uses custom 32-bit counter for precise timing, subsecond tick support, and efficient alarm scheduling
- **Global State**: `movement_state_t` stores settings in RTC backup registers (persist through deep sleep)
- **Timezone Support**: Integrated with `utz` library for DST-aware timezone calculations

Key Movement functions watch faces use:
- `movement_move_to_face()` / `movement_move_to_next_face()` - Navigation
- `movement_request_tick_frequency()` - Request faster/slower ticks (default 1Hz)
- `movement_schedule_background_task()` - Schedule wake for specific time
- `movement_illuminate_led()` / `movement_force_led_on()` - LED control
- `movement_play_signal()` / `movement_play_alarm()` - Sound output
- `movement_get_local_date_time()` / `movement_set_local_date_time()` - Time/date access

### Watch Face Structure

Watch faces are modular applications with a standard lifecycle:

```c
typedef struct {
    watch_face_setup setup;      // Initialize context, called at boot and after deep sleep
    watch_face_activate activate; // Prepare to go on-screen, enable peripherals
    watch_face_loop loop;         // Handle events, update display
    watch_face_resign resign;     // Clean up before going off-screen
    watch_face_advise advise;     // Optional: request background tasks
} watch_face_t;
```

Watch faces must:
- Call `movement_move_to_next_face()` on `EVENT_MODE_BUTTON_UP` or `EVENT_MODE_LONG_PRESS`
- Return tick frequency to 1Hz in `resign()` if changed
- Not wake peripherals during `EVENT_LOW_ENERGY_UPDATE`
- Store state in a custom context struct allocated in `setup()`

### Key Event Types

- `EVENT_ACTIVATE` - Face entering foreground
- `EVENT_TICK` - Regular update (frequency configurable, default 1Hz)
- `EVENT_TIMEOUT` - Inactivity timeout, should consider resigning
- `EVENT_LOW_ENERGY_UPDATE` - Once-per-minute update in sleep mode
- `EVENT_BACKGROUND_TASK` - Scheduled background task execution
- `EVENT_LIGHT_BUTTON_UP/DOWN`, `EVENT_MODE_BUTTON_UP/DOWN`, `EVENT_ALARM_BUTTON_UP/DOWN` - Button events
- `EVENT_LIGHT_LONG_PRESS`, `EVENT_MODE_LONG_PRESS`, `EVENT_ALARM_LONG_PRESS` - Long press events

### Watch Face Organization

Watch faces live in `watch-faces/` organized by category:
- `clock/` - Time display faces (clock, world clock, mars time, etc.)
- `complication/` - Utility faces (alarm, stopwatch, countdown, etc.)
- `sensor/` - Hardware sensor faces (temperature, accelerometer, voltage)
- `settings/` - Configuration faces (set time, preferences)
- `demo/` - Test/diagnostic faces
- `io/` - Input/output faces (UART, IR)

To add a new watch face:
1. Create `.c` and `.h` files in appropriate `watch-faces/` subdirectory
2. Add source file to `watch-faces.mk`
3. Add header include to `movement_faces.h`
4. Add face instance to `watch_faces[]` array in `movement_config.h`

Use `template/template.c` and `template/template.h` as starting points. The Python script `template/watch_face.py` can automate face creation.

### Watch Library (watch-library/)

The watch library provides hardware abstraction:
- `hardware/watch/` - Real hardware implementations
- `simulator/watch/` - Emscripten-based simulator implementations
- `shared/watch/` - Common code and driver modules
- `shared/driver/` - Hardware drivers (LIS2DW accelerometer, thermistor)

Critical implementation detail: This project uses `rtc32.c` (32-bit counter) instead of gossamer's standard `rtc.c`. The Makefile explicitly excludes gossamer's RTC implementation.

Key watch library modules:
- `watch_rtc.h` - Real-time clock interface (backed by 32-bit counter)
- `watch_slcd.h` - Segment LCD display control
- `watch_gpio.h`, `watch_extint.h` - GPIO and external interrupts (buttons)
- `watch_i2c.h`, `watch_spi.h`, `watch_uart.h` - Communication peripherals
- `watch_adc.h` - Analog input (light sensor, voltage monitoring)
- `watch_deepsleep.h` - Deep sleep mode management
- `watch_storage.h` - Flash storage access

### Configuration Files

- `movement_config.h` - Defines which faces to include, default settings, LED colors, timeouts
- `movement_faces.h` - Includes all watch face headers
- `watch-faces.mk` - Lists all watch face source files for compilation
- `movement_custom_signal_tunes.h` - Custom buzzer tunes for hourly chime

Editing `movement_config.h` is the primary way to customize the firmware build.

### Additional Modules

- `filesystem/` - LittleFS integration for flash file storage
- `shell/` - USB serial command shell for debugging/configuration
- `utz/` - Timezone calculation library with DST support
- `lib/` - Third-party libraries (TOTP, SHA, base32/64, sunriset astronomy, chirpy IR)

### Build System

Uses GNU Make with gossamer framework:
- `Makefile` - Main build configuration
- `gossamer/make.mk`, `gossamer/rules.mk` - Gossamer framework makefiles
- Build outputs to `build/` for hardware, `build-sim/` for emulator

The build system conditionally includes hardware or simulator watch library implementations based on the `EMSCRIPTEN` flag.

## Development Notes

### Button Event Handling

Movement implements sophisticated button handling with long-press detection and debouncing:
- Button down events set timestamps in 32-bit counter space
- Long press threshold is `MOVEMENT_LONG_PRESS_TICKS` (64 ticks = ~2 seconds at 32Hz)
- Optional debouncing via `MOVEMENT_DEBOUNCE_TICKS` in `movement_config.h`
- Events are queued as bitmask in `movement_volatile_state.pending_events`

Watch faces receive button events in the order: DOWN → UP/LONG_PRESS → LONG_UP

### Efficient Event Processing

Recent optimizations use `__builtin_ctz()` (count trailing zeros) for O(1) event extraction from the pending events bitmask instead of linear scanning.

### Background Tasks

Watch faces can schedule background tasks via `movement_schedule_background_task()`. Movement wakes the face with `EVENT_BACKGROUND_TASK` at the scheduled time, even if not in foreground. Use sparingly to preserve battery.

### LED and Buzzer Priority

Movement manages LED and buzzer conflicts:
- LED: Instant light, button presses, and manual control coordinate via state machine
- Buzzer: Priority system (`BUZZER_PRIORITY_BUTTON` < `BUZZER_PRIORITY_SIGNAL` < `BUZZER_PRIORITY_ALARM`)
- Recent refactor decouples LED and buzzer to allow simultaneous operation

### Power Efficiency

The firmware aggressively uses deep sleep modes:
- STANDBY mode between ticks (most peripherals powered down)
- Low-energy mode after configurable inactivity (only RTC remains active)
- Movement automatically disables peripherals when entering sleep
- Watch faces must restore peripheral state in `activate()` after sleep

### Testing in Emulator

The emulator (`EMSCRIPTEN=1` builds) provides rapid iteration without hardware. Key differences from hardware:
- Simulated segment LCD rendered in browser
- Button clicks via web UI
- No real I2C/SPI peripherals (stubs provided)
- Faster than real-time execution possible

Build and run: `emmake make BOARD=sensorwatch_red DISPLAY=classic && python3 -m http.server -d build-sim`

## Common Pitfalls

- **Forgetting to call `movement_move_to_next_face()`**: User gets stuck on your face. Always handle `EVENT_MODE_BUTTON_UP` or `EVENT_MODE_LONG_PRESS`.
- **Not resetting tick frequency**: If you request faster ticks, you must reset to 1Hz in `resign()`.
- **Waking peripherals in low-energy mode**: Never enable I2C, SPI, ADC, etc. during `EVENT_LOW_ENERGY_UPDATE`.
- **Ignoring BOARD/DISPLAY requirements**: Most make targets fail without these parameters.
- **Not syncing submodules**: Missing gossamer or other submodules cause cryptic build errors.
- **Mixing up RTC implementations**: This project uses `rtc32.c`, not gossamer's standard `rtc.c`.

## Key Files Reference

- `movement.c` (1481 lines) - Core application loop and face management
- `movement.h` - Movement API, event types, settings structures
- `movement_config.h` - User-editable configuration (which faces, defaults)
- `watch-faces.mk` - List of all compiled watch face sources
- `movement_faces.h` - Includes for all watch face headers
- `watch-library/hardware/watch/rtc32.c` - Custom 32-bit RTC counter implementation
