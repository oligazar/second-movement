# Tomato Face Documentation

## Overview

The Tomato Face is a Pomodoro Technique timer for Sensor Watch, implementing the popular time management method with 25-minute work sessions, 5-minute short breaks, and 20-minute long breaks.

## Display Layout

```
FO  0  25  00  4
││  │  │   │   └─ Round within cycle (1-4)
││  │  │   └───── Seconds
││  │  └───────── Minutes
││  └──────────── Total completed pomodoros (cumulative)
│└─────────────── Phase: FO (Focus), br (Short Break), Br (Long Break)
```

## Session Types

- **Focus (FO)**: 25 minutes - Work session
- **Short Break (br)**: 5 minutes - After each focus session
- **Long Break (Br)**: 20 minutes - After 4 focus sessions

## Button Controls

### When Timer is Stopped

| Button | Action |
|--------|--------|
| ALARM (short press) | Cycle through phases (FO → br → Br → FO) |
| ALARM (long press) | **Start the timer** |
| LIGHT (long press) | Reset pomodoro count to 0 |

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

## Indicators

- **BELL**: Solid when running, blinks when paused
- **COLON**: Blinks when timer is running (custom LCD only, hardware-based)
- **LAP**: Autorun mode enabled

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

## Recent Optimizations (2025)

1. **Improved timeout handling** - Running timers no longer timeout
2. **Low energy mode support** - Graceful display in power-saving mode
3. **Colon blinking on custom LCD** - Hardware-based visual feedback
4. **Paused display optimization** - Reduced CPU usage when paused

## Configuration

Session durations are configurable via static variables in `tomato_face.c`:
```c
static uint8_t focus_min = 25;
static uint8_t break_min = 5;
static uint8_t long_break_min = 20;
static uint8_t rounds = 4;  // Number of pomodoros until long break
```

## Credits

Original implementation by Wesley Ellis (2022)
Ported to second-movement with optimizations (2025)
