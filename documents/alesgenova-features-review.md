# Feature Review: alesgenova/rtc-counter32-my-build Branch

This document reviews interesting features from Alessandro Genova's custom build that could be cherry-picked for your custom build branch.

## Branch Overview
- **Base**: 49 commits ahead of current branch
- **Total changes**: 82 files changed, 11,937 insertions(+), 1,403 deletions(-)
- **Focus**: Enhanced user experience, better customization, performance optimizations

---

## 🌟 Highly Recommended Features

### 1. **Page Ordering Face** (`page_ordering_face`)
**Location**: `watch-faces/settings/page_ordering_face.{c,h}`

**What it does**:
- Runtime face reordering without recompiling firmware
- Enable/disable individual faces on the fly
- Set secondary/tertiary face indices dynamically
- Complete control over face organization

**Usage**:
- Normal mode:
  - ALARM: Select next page
  - ALARM long: Select previous page
  - LIGHT: Enable/disable current page
  - LIGHT long: Enter reordering mode
- Reordering mode (page number flashing):
  - ALARM: Move face to next page
  - ALARM long: Move face to previous page
  - LIGHT long: Mark as first secondary page
  - LIGHT: Exit reordering mode

**Why it's great**: Users can customize their watch without rebuilding firmware. Perfect for testing different face orders.

**Implementation details**:
- Adds global arrays: `page_to_face[]`, `face_to_page[]`, `watch_face_status[]`
- Modifies movement.c to support dynamic face navigation
- Face enable/disable persists across reboots

---

### 2. **Tertiary Face Set Support**
**Commits**: `ab8f169`, `474ebb9`

**What it does**:
- Adds third tier of faces beyond primary/secondary
- MOVEMENT_TERTIARY_FACE_INDEX config option
- Integrated with page_ordering_face

**Navigation**:
- Short MODE press from primary face → secondary faces
- Another short MODE → tertiary faces
- Really long press (configurable) → jump directly to tertiary

**Why it's great**: Better organization for large face collections (settings, diagnostics, rarely-used features)

**Config example**:
```c
#define MOVEMENT_SECONDARY_FACE_INDEX 6   // Start of secondary faces
#define MOVEMENT_TERTIARY_FACE_INDEX 11   // Start of tertiary faces (settings)
```

---

### 3. **Really Long Press Detection**
**Commits**: `474ebb9`

**What it does**:
- Adds `MOVEMENT_REALLY_LONG_PRESS_TICKS` (192 ticks = ~6 seconds at 32Hz)
- New events: `EVENT_MODE_REALLY_LONG_PRESS`, `EVENT_LIGHT_REALLY_LONG_PRESS`, `EVENT_ALARM_REALLY_LONG_PRESS`
- Used for tertiary face navigation

**Why it's great**: Expands button vocabulary for power users without interfering with existing gestures

---

### 4. **Stock Clock Face with Quick Timer**
**Location**: `watch-faces/clock/stock_clock_face.{c,h}`

**What it does**:
- Replicates classic F91W clock behavior
- Built-in quick countdown timer accessible from main clock
- Displays remaining time in seconds field
- Delegates hourly chime to separate face (cleaner separation)
- ALARM button toggles 12/24H display

**Quick timer usage**:
- ALARM long press: Enable/disable timer
- ALARM short press (when active): Cycle through preset durations
- Counts down in seconds field
- Plays alarm when complete

**Preset durations**: 1, 3, 5, 10, 15, 20, 30, 45, 60, 90, 120 minutes

**Why it's great**: Combines main clock with quick timer (like Pomodoro face but simpler). Very convenient for everyday use.

---

### 5. **Dynamic Tunes System**
**Commits**: `aa4b55e`
**Location**: `movement_tunes_config.h`, `tunes_face.{c,h}`, `movement_custom_alarm_tunes.{c,h}`

**What it does**:
- Include multiple signal/alarm tunes in single build
- Select desired tune at runtime via `tunes_face`
- No recompilation needed to change tunes
- Separate volume controls for signals vs alarms

**Configuration**:
```c
// movement_tunes_config.h - Include desired tunes
#define INCLUDE_SIGNAL_TUNE_ZELDA_SECRET
#define INCLUDE_SIGNAL_TUNE_MARIO_THEME
#define INCLUDE_ALARM_TUNE_NOKIA
#define INCLUDE_ALARM_TUNE_BAD_APPLE
// ... 50+ tunes available!
```

**Why it's great**: No more choosing one tune at compile time. Switch between Nokia, Zelda, Mario, Touhou, etc. on the watch!

---

### 6. **Enhanced Alarm Face Improvements**
**Commits**: `31cbc06`, `b122c8c`

**What it does**:
- Better default alarm tune (not too slow)
- Independent volume controls (button/signal/alarm)
- More reliable alarm firing
- Improved alarm face UX

**New volume settings**:
```c
#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_SOFT
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_ALARM_VOLUME WATCH_BUZZER_VOLUME_LOUD
```

**Why it's great**: Allows quiet button feedback but loud alarms. Better user control.

---

### 7. **Wake-Up on Any Button**
**Commits**: `b918365`

**What it does**:
- Exit deep sleep with any button press (not just ALARM)
- More intuitive wake-up behavior
- Faster return from low-energy mode

**Why it's great**: Eliminates confusion when watch appears "stuck" in sleep mode.

---

### 8. **PIN Lock System**
**Location**: `watch-faces/settings/pin_face.{c,h}`, `movement_pin_service.{c,h}`

**What it does**:
- 6-digit PIN protection
- Auto-lock after configurable timeout (default 5 min)
- Change PIN via face UI
- Lock/unlock without leaving face

**PIN entry encoding**:
- MODE_DOWN → 0
- MODE_LONG → 1
- LIGHT_DOWN → 2
- LIGHT_LONG → 3
- ALARM_DOWN → 4
- ALARM_LONG → 5

**Why it's great**: Security feature for sensitive watch faces (TOTP, personal data). Industrial/enterprise use cases.

---

### 9. **Optional Button Debouncing**
**Commits**: `498faf6`

**What it does**:
- Configurable `MOVEMENT_DEBOUNCE_TICKS` in `movement_config.h`
- Prevents accidental double-presses
- Useful for worn buttons or shaky hands

**Configuration**:
```c
#define MOVEMENT_DEBOUNCE_TICKS 2  // ~60ms debounce at 32Hz
```

**Why it's great**: Improves button reliability for watches with worn hardware.

---

## 🔧 Performance & Architecture Improvements

### 10. **Event Processing Optimization**
**Commits**: `ba8e01d`, `1e93388`

**What it does**:
- Uses `__builtin_ctz()` for O(1) event extraction from bitmask
- Immediately process events when exiting deep sleep
- More efficient event queue handling

**Why it's great**: Faster event response, lower latency. Good foundation for responsive UI.

---

### 11. **Decoupled LED and Buzzer Control**
**Commits**: `51ab1c7`, `4727d60`

**What it does**:
- LED and buzzer can operate simultaneously
- Fixes LED flickering during buzzer playback
- Independent priority systems

**Why it's great**: Better hardware utilization, smoother UX during alarms/signals.

---

### 12. **Non-Blocking Buzzer Playback**
**Commits**: `6b5b6b3`, `beb08cf`

**What it does**:
- `watch_buzzer_play_sequence()` no longer blocks
- Arbitrary tune streaming
- Better simulator beep reliability

**Why it's great**: Watch remains responsive during tune playback. Fixes faces that relied on blocking behavior.

---

### 13. **Fast Stopwatch Power Efficiency**
**Commits**: `f29ed44`, `5a97f92`, `b737bf8`

**What it does**:
- Uses rtc-counter32 for microsecond precision
- Slow display refresh mode option
- More efficient display drawing
- Power-efficient despite high precision

**Why it's great**: High-precision timing without battery drain. Demonstrates rtc32 advantages.

---

### 14. **Improved RTC Counter32 Stability**
**Commits**: `1bd1d49`, `633eb13`, `d04b775`, `1872ea3`, `f03a4f9`

**What it does**:
- Fixes corner cases causing missed minute alarms
- Aligns top-of-second with 1Hz interrupt
- Minimal work in interrupt callbacks
- Removes unnecessary workarounds

**Why it's great**: More reliable timekeeping. Critical for alarm faces.

---

### 15. **Mode Button Down Events**
**Commits**: `9e127d1`, `ba8e01d`, `61d32e8`

**What it does**:
- Faces receive `EVENT_MODE_BUTTON_DOWN`
- Prevents event forwarding to new face after navigation
- Better button event semantics

**Why it's great**: Enables new interaction patterns. Prevents accidental actions on face transitions.

---

## 🛠️ Developer/Debug Tools

### 16. **RTC Counter Debug Face**
**Location**: `watch-faces/demo/rtccount_face.{c,h}`

**What it does**:
- Displays raw counter32 metrics
- Debug timing issues
- Monitor counter behavior

**Why it's great**: Essential for debugging rtc32 timing problems.

---

### 17. **Subsecond Precision in Set Time Face**
**Commits**: `6c09607`

**What it does**:
- Set time with subsecond accuracy
- Useful for synchronizing multiple watches
- Better initial time setting

**Why it's great**: Precision matters for stopwatch, fine-tuning, and sync scenarios.

---

## 📊 Feature Priority Recommendations

### Must Have (High Value, Low Risk):
1. ✅ **Page Ordering Face** - Killer feature for customization
2. ✅ **Stock Clock with Quick Timer** - Practical daily use
3. ✅ **Dynamic Tunes System** - Fun and useful
4. ✅ **Wake-Up on Any Button** - UX improvement
5. ✅ **Independent Alarm/Signal Volume** - Better control

### Should Have (Good Value):
6. ✅ **Tertiary Face Set** - Better organization
7. ✅ **Really Long Press** - Extended vocabulary
8. ✅ **Event Processing Optimizations** - Performance win
9. ✅ **Non-Blocking Buzzer** - Responsiveness
10. ✅ **Fast Stopwatch Improvements** - If using stopwatch

### Consider (Use-Case Dependent):
11. ⚠️ **PIN Lock System** - Only if needed for security
12. ⚠️ **Button Debouncing** - Only if buttons are problematic
13. ⚠️ **Mode Button Down Events** - Advanced face development
14. ⚠️ **Subsecond Set Time** - Precision timing needs

### Lower Priority (Nice to Have):
15. 🔽 **RTC Counter Debug Face** - Development/debugging only
16. 🔽 **LED/Buzzer Decoupling** - Subtle improvement

---

## 🎯 Suggested Cherry-Pick Strategy

### Phase 1: Core Infrastructure
```bash
# Foundation improvements (order matters!)
git cherry-pick afdc403  # Initial stable rtc-counter32
git cherry-pick 1872ea3  # Minimal interrupt work
git cherry-pick ba8e01d  # __builtin_ctz optimization
```

### Phase 2: Button Enhancements
```bash
git cherry-pick 474ebb9  # Really long press
git cherry-pick b918365  # Wake-up any button
git cherry-pick 9e127d1  # Mode button down events
```

### Phase 3: Face Organization
```bash
git cherry-pick ab8f169  # Tertiary face set
git cherry-pick 3b0d191  # Dynamic faces (page ordering)
```

### Phase 4: Sound System
```bash
git cherry-pick aa4b55e  # Dynamic tunes
git cherry-pick 31cbc06  # Alarm improvements
git cherry-pick f3f69b2  # Independent volumes
git cherry-pick 51ab1c7  # Decouple LED/buzzer
git cherry-pick 6b5b6b3  # Non-blocking buzzer
```

### Phase 5: New Faces
```bash
git cherry-pick f67cd97  # Stock clock with quick timer
# Also includes tunes_face, page_ordering_face
```

### Optional Additions:
```bash
git cherry-pick 498faf6  # Button debouncing (if needed)
git cherry-pick e912e6d  # PIN service (if security needed)
git cherry-pick f29ed44  # Fast stopwatch improvements
```

---

## ⚠️ Integration Challenges

### Conflicts to Expect:
1. **movement.c/h**: Heavy modifications to core event system
2. **movement_config.h**: Different face selection and settings
3. **movement_custom_signal_tunes.h**: Restructured for dynamic loading
4. **watch_rtc.{c,h}**: RTC counter32 implementation changes
5. **watch_tcc.{c,h}**: Buzzer/LED decoupling

### Testing Requirements:
- Verify all existing faces still work
- Test button timing (long press, really long press)
- Confirm alarm reliability (minute alarm corner cases)
- Check power consumption (especially with new features)
- Test face enable/disable in page_ordering_face
- Verify dynamic tune switching

---

## 🎬 Quick Start: Minimal Integration

If you want just the best features quickly:

```bash
# 1. Stock clock with quick timer
git show alesgenova/rtc-counter32-my-build:watch-faces/clock/stock_clock_face.c > watch-faces/clock/stock_clock_face.c
git show alesgenova/rtc-counter32-my-build:watch-faces/clock/stock_clock_face.h > watch-faces/clock/stock_clock_face.h

# 2. Add to watch-faces.mk
echo "watch-faces/clock/stock_clock_face.c \\" >> watch-faces.mk

# 3. Update movement_faces.h
# Add: #include "clock/stock_clock_face.h"

# 4. Update movement_config.h watch_faces[]
# Replace clock_face with stock_clock_face
```

---

## 📝 Notes

- Alessandro's branch is actively maintained and stable
- Many commits are small, focused improvements (good for cherry-picking)
- Extensive testing with rtc-counter32 optimizations
- Custom build includes 50+ alarm tunes and 20+ signal tunes
- Some features (PIN, page ordering) require global state additions

---

## 🔗 Related Commits

For detailed implementation, examine these commit ranges:
- RTC Counter32 foundation: `afdc403` → `cce0818`
- Button system overhaul: `498faf6` → `9e127d1`
- Sound system improvements: `6b5b6b3` → `aa4b55e`
- Face organization: `3b0d191` → `ab8f169`
- Custom build config: `69409ac`, `797ae9c`

---

## Summary

Alessandro's branch represents a mature, well-tested enhancement of the second-movement firmware with focus on:
1. **User customization** (page ordering, dynamic tunes)
2. **Practical features** (quick timer, better alarms)
3. **Performance** (event optimization, power efficiency)
4. **Developer experience** (mode button events, debug tools)

The most valuable additions are **page_ordering_face**, **stock_clock_face with quick timer**, and **dynamic tunes system**. These significantly improve daily usability without major architectural risks.
