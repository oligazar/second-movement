# Tomato Face Documentation

## Overview

The Tomato Face is a Pomodoro Technique timer for Sensor Watch, implementing the popular time management method with customizable work sessions and breaks. Features a 2-preset system for switching between different Pomodoro configurations (e.g., standard 25/5/20 vs long 50/10/30).

## Display Layout

### When Running
```
FO1  1  17:00  5
│││  │  ││ ││  └─ Total completed work sessions (cumulative count)
│││  │  ││ └───── Seconds (00-59)
│││  │  │└─────── Decimal point (custom LCD only)
│││  │  └──────── Minutes (0-60)
│││  └─────────── Round within current cycle (1-4)
││└────────────── Preset number (1-2)
│└─────────────── Phase: FO (Focus), br (Short Break), BR (Long Break)
```

### When Stopped

**Summary View** (default when stopped):
```
TO1  4  25 05 20
│││  │  │  │  └─ Long break duration (minutes)
│││  │  │  └──── Short break duration (minutes)
│││  │  └─────── Focus duration (minutes)
│││  └────────── Cycles until long break (rounds)
││└───────────── Preset number (1-2)
│└────────────── Tomato preset indicator
```

**Phase Preview** (cycle with LIGHT button):
```
FO1  0  25:00  0
│││  │  ││ ││  └─ Total completed work sessions
│││  │  ││ └───── Seconds (always 00 when stopped)
│││  │  │└─────── Decimal point/colon separator
│││  │  └──────── Duration in minutes
│││  └─────────── Round number (0 until first session completes)
││└────────────── Preset number (1-2)
│└─────────────── Phase to preview: FO (Focus), br (Short Break), LB (Long Break)
```

Phase preview shows exactly what the display will look like when running, minus the bell icon.

## Preset System

Two customizable presets stored in flash memory:
- **Preset 1** (default): 25/5/20 minutes, 4 rounds - Standard Pomodoro
- **Preset 2** (default): 50/10/30 minutes, 4 rounds - Long sessions

Toggle between presets with **ALARM long press** when stopped. Active preset indicated by number in phase label (FO1, br2, BR1, etc.)

## Session Types

- **Focus (FO/FO1/FO2)**: Work session (uppercase for emphasis)
- **Short Break (br/br1/br2)**: After each focus session (lowercase, less prominent)
- **Long Break (BR/BR1/BR2)**: After completing all rounds in cycle (uppercase)

## Button Controls

### When Timer is Stopped

| Button | Action |
|--------|--------|
| LIGHT (short press) | Cycle through phases (FO → br → BR → FO) |
| LIGHT (long press) | **Enter settings mode** |
| ALARM (short press) | **Start the timer** from current phase |
| ALARM (long press) | **Toggle between presets** (1 ↔ 2) |

### When Timer is Running

| Button | Action |
|--------|--------|
| ALARM (short press) | Pause the timer |
| ALARM (long press) | Toggle autorun mode on/off |

### When Timer is Paused

| Button | Action |
|--------|--------|
| ALARM (short press) | Resume the timer |
| ALARM (long press) | Reset to stopped state |

### Settings Mode

Enter settings mode with **LIGHT long press** when timer is stopped. Settings apply to the **active preset only**.

| Button | Action |
|--------|--------|
| LIGHT (short press) | Cycle to next setting (exits after last setting) |
| ALARM (short press) | Increment current setting value |
| ALARM (long press) | Enable quick increment (hold to rapidly increase) |

**Configurable Settings (per preset):**
1. **FOCUS** / **St** (Focus duration): 1-60 minutes
2. **break** / **br** (Short break duration): 1-30 minutes
3. **LnBrk** / **Br** (Long break duration): 1-60 minutes
4. **CYCLE** / **Cy** (Cycles until long break): 1-10 rounds
5. **COUNT** / **Ct** (Total count): Reset to 0 (ALARM press resets immediately)

The current value blinks to indicate which setting is being edited. All settings save immediately to flash and persist across power cycles.

## Indicators

- **BELL**: Solid when running, blinks when paused
- **COLON**: Shows between minutes and seconds when running (classic LCD only, hardware-based)
- **DECIMAL POINT**: Shows between minutes and seconds when running (custom LCD only, hardware-based)
- **LAP** (Looped arrow on custom, "LAP" text on classic): Autorun mode enabled

## Features

### Autorun Mode

When autorun is enabled (LAP indicator on):
- Timer automatically starts the next session when current one completes
- Focus → Short Break → Focus (repeat)
- After 4 focus sessions: Focus → Long Break → Focus

Toggle autorun with ALARM long press while timer is running.

### Background Task Support

The timer continues running even when you switch to other watch faces. You'll receive an alarm notification when the session completes.

### Low Energy Mode Behavior

**When timer is running:**
- Display shows phase + minutes + "--" for seconds
- Updates once per minute
- Custom LCD: Colon blinks (hardware, no power cost)
- Classic LCD: Sleep animation displays
- Timer stays on face

**When timer is paused or stopped:**
- Face navigates back to clock face for consistency

### Power Management

**Timeout behavior:**
- Running timer: Prevents timeout, stays visible
- Paused/stopped timer: Allows timeout after inactivity interval

**Display optimization:**
- When paused, only BELL indicator updates (no full redraw)
- Saves CPU cycles and battery

## Recent Enhancements (2025)

### Phase 1 (Early 2025)
1. **Improved timeout handling** - Running timers no longer timeout
2. **Low energy mode support** - Graceful display in power-saving mode
3. **Colon blinking** - Hardware-based visual feedback
4. **Paused display optimization** - Reduced CPU usage when paused
5. **Settings mode** - User-configurable durations without code changes
6. **Code refactoring** - Improved readability with extracted helper functions

### Phase 2 (November 2025)
7. **2-preset system** - Two customizable Pomodoro configurations with flash storage
8. **Preset toggle** - Quick switching between presets (ALARM long press when stopped)
9. **Enhanced display** - Shows all durations when stopped for better overview
10. **Display optimization** - Proper use of `watch_display_text_with_fallback()` for classic vs custom LCD
11. **Decimal point separator** - Visual clarity on custom LCD (17:00 instead of 1700)
12. **LAP indicator persistence** - Autorun state now restores after face navigation
13. **Phase preview** - Cycle through phases when stopped to see what's next
14. **Count reset** - Added as 5th setting for easier pomodoro tracking restart

## Configuration

Session durations are **user-configurable per preset** via the settings mode (see Button Controls above). Enter settings mode with **LIGHT long press** when the timer is stopped, then adjust the active preset:
- Focus duration (1-60 minutes)
- Short break duration (1-30 minutes)
- Long break duration (1-60 minutes)
- Number of cycles until long break (1-10 rounds)
- Total count (reset to 0)

### Default Preset Values
- **Preset 1**: 25/5/20 minutes, 4 rounds (Standard Pomodoro)
- **Preset 2**: 50/10/30 minutes, 4 rounds (Long sessions)

All settings persist in **flash storage** (survives power cycles and firmware updates). Default values can be changed in `_tomato_load_presets()` in `tomato_face.c` if needed.

## Technical Notes

### Display Differences: Classic vs Custom LCD

The face adapts to both LCD types using `watch_display_text_with_fallback()`:

**Classic LCD (F-91W)**:
- Phase labels: 2 characters (F1, b1, B1)
- Timer format: "1700 1" (no separator, 6 chars)
- Top row constraint: 2 chars left + 2 chars right

**Custom LCD (A158WEA-9)**:
- Phase labels: 3 characters (FO1, br1, BR1)
- Timer format: "17:00 1" (decimal point separator, 6 chars)
- Top row constraint: 3 chars left + 2 chars right (OOO oo layout)
- Decimal point: Separate LCD segment at colon position, controlled by `watch_set_decimal_if_available()`

### Round vs Count Display

- **Top right**: Round number **within current cycle** (1-4), calculated via modulo: `count % rounds`
- **Bottom right**: Total work sessions **cumulative count** (increases indefinitely)
- Example: After 5 completed sessions with 4-round cycles → Top: 1, Bottom: 5

## Credits

Original concept and implementation by Wesley Ellis (2022)

Significantly enhanced and ported to second-movement by Oleksandr Kostenko (2025):
- **Phase 1**: Pause/resume, long breaks, autorun mode, settings, low energy mode (~555 lines)
- **Phase 2**: 2-preset system with flash storage, enhanced display, decimal point separator, phase preview (~650 lines)
