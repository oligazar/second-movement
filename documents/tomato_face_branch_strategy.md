# Tomato Face Branch Maintenance Strategy

**Documentation Date:** November 9, 2025
**Status:** Plan documented, not yet implemented
**Current Branch:** `feature/pomodoro`

## Context

### Repository Structure
- **Your fork:** `oligazar/second-movement`
- **Upstream:** `joeycastillo/second-movement` (main second-movement project)
- **Collaborator:** `alesgenova/second-movement` (rtc32 and other experimental branches)

### Current State
- **Branch:** `feature/pomodoro` based on `main`
- **Commits:** 5 commits ahead of main with complete tomato_face implementation
- **Status:** Fully functional with settings mode, optimizations, and documentation

### Goals
- Keep tomato_face mergeable to upstream (Joey's main branch)
- Prepare for eventual rtc32 merge from Alessandro's work
- Minimize maintenance overhead (single branch preferred)
- Eventually contribute to upstream

## Recommended Strategy: Single Branch with Forward Compatibility

**Principle:** Write code that works on current main but automatically benefits from rtc32 features when available.

### Why This Approach?
- ✅ **Single codebase** - no branch juggling or code duplication
- ✅ **Works everywhere** - on main now, rtc32 later, both from same code
- ✅ **Zero migration cost** - no rewrite needed when rtc32 merges
- ✅ **Contribution-ready** - can PR to upstream immediately
- ✅ **Future-proof** - automatically gains new features when available

## RTC32 Branch Analysis

### What Alessandro's rtc32 Branch Adds

**Architecture Changes:**
- Direct unix timestamp storage in 32-bit counter
- `movement_get_utc_timestamp()` - direct timestamp access
- `watch_rtc_get_unix_time()` / `watch_rtc_set_unix_time()` - unix time RTC functions
- Cached date_time conversions to avoid redundant conversions within same second

**User-Facing APIs:**
- `movement_button_should_sound()` - checks user preference for button sounds
- `movement_button_volume()` - gets user-configured button volume

### Applicable Improvements for Tomato Face

**Can adopt without architecture changes:**
- ✅ **Button beep helper** - respects user sound/volume preferences

**Cannot adopt without rtc32 architecture:**
- ❌ **Direct timestamp API** - requires rtc32 RTC implementation
- ❌ **Cached conversions** - requires rtc32 RTC implementation

**Low-value improvements (not worth effort):**
- ❌ Static quick_ticks flag instead of state field
- ❌ Tap detection support
- ❌ Inline function hints

## Implementation Plan (Future)

### Step 1: Add Forward-Compatible Button Beep Helper

**Location:** `watch-faces/complication/tomato_face.c`

Add after the sound sequence definitions (around line 50):

```c
// Forward-compatible button beep helper
// Works on main branch (fallback) and automatically uses rtc32 features when available
static inline void button_beep() {
    #ifdef MOVEMENT_BUTTON_VOLUME_AVAILABLE
    // When rtc32 is merged, this respects user preferences
    if (movement_button_should_sound())
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    #else
    // Fallback for current main branch - always beep
    watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
    #endif
}
```

**How it works:**
- On current main: `MOVEMENT_BUTTON_VOLUME_AVAILABLE` undefined → uses fallback → works exactly as today
- After rtc32 merge: Define the flag → automatically uses new API → respects user preferences
- Zero code changes needed when rtc32 merges

### Step 2: Replace Hardcoded Beeps

**Current locations with hardcoded beeps in tomato_face.c:**

| Line | Function | Context |
|------|----------|---------|
| 145 | `_cycle_phase_forward()` | 3 beep instances for phase cycling |
| 181 | `_handle_settings_mode()` | LIGHT_BUTTON_UP exit settings |
| 183 | `_handle_settings_mode()` | LIGHT_BUTTON_UP cycle setting |
| 189 | `_handle_settings_mode()` | ALARM_BUTTON_UP increment value |
| 202 | `_handle_settings_mode()` | ALARM_LONG_UP stop quick ticks |
| 443 | `EVENT_LIGHT_BUTTON_UP` | Cycle phases (via `_cycle_phase_forward`) |
| 450 | `EVENT_LIGHT_LONG_PRESS` | Reset count |
| 494 | `EVENT_ALARM_LONG_PRESS` | Toggle autorun/reset |

**Replace pattern:**
```c
// OLD:
watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);

// NEW:
button_beep();
```

**Keep alarm/notification sounds unchanged:**
- `_sound_seq_alarm` - Timer completion (not a button press)
- `_sound_seq_double_beep` - Timer start confirmation
- `_sound_seq_pause` - Pause/resume state change feedback
- `_sound_seq_beep` - High beep for significant actions (consider case-by-case)

### Step 3: When rtc32 Merges to Upstream

At that time, add to `movement_config.h` or `movement.h`:

```c
// Indicates movement_button_should_sound() and movement_button_volume() are available
#define MOVEMENT_BUTTON_VOLUME_AVAILABLE
```

**No other code changes needed** - conditional compilation handles everything.

## Git Workflow

### Current Development
```bash
# Continue working on feature/pomodoro
git checkout feature/pomodoro

# Make changes, commit normally
git add .
git commit -m "your changes"
```

### Keeping Up-to-Date with Upstream
```bash
# Periodically sync main with upstream
git checkout main
git pull upstream main
git push origin main

# Rebase feature branch onto updated main
git checkout feature/pomodoro
git rebase main
# Resolve any conflicts if they arise
git push origin feature/pomodoro --force-with-lease
```

### Testing Against rtc32 (Optional Verification)
```bash
# Create temporary test branch
git checkout -b test-rtc32-compat alesgenova/rtc-counter32

# Cherry-pick your tomato commits
git cherry-pick <commit-hash-1>
git cherry-pick <commit-hash-2>
# ... etc

# Test that tomato_face works
make BOARD=sensorwatch_red DISPLAY=classic

# Discard branch when done
git checkout feature/pomodoro
git branch -D test-rtc32-compat
```

### When Ready to Contribute
```bash
# Push to your fork
git push origin feature/pomodoro

# Create PR to upstream via GitHub web interface:
# From: oligazar/second-movement feature/pomodoro
# To: joeycastillo/second-movement main
```

## Alternative Strategies (Considered but Rejected)

### Option A: Multi-Branch Approach
```
feature/pomodoro-main    → for upstream (no rtc32 dependencies)
feature/pomodoro-rtc32   → with rtc32 enhancements
```

**Advantages:**
- Separate concerns cleanly
- Can use rtc32 features immediately on rtc32 branch

**Disadvantages:**
- More maintenance overhead (2 branches to keep in sync)
- Code duplication
- Cherry-picking changes between branches

**Rejected because:** User prefers single branch, more maintenance overhead

### Option B: Wait for rtc32 Merge
Wait until rtc32 merges to upstream, then implement tomato_face with all features.

**Advantages:**
- Single implementation with all features
- No compatibility concerns

**Disadvantages:**
- Unknown timeline for rtc32 merge
- Can't use or refine tomato_face in the meantime

**Rejected because:** Want to use tomato_face now, unknown wait time

### Option C: Fork Alessandro's Branch
Base work on `alesgenova/rtc-counter32` instead of main.

**Advantages:**
- Immediate access to rtc32 features
- Can optimize for rtc32 architecture

**Disadvantages:**
- Not mergeable to upstream main
- Defeats contribution goal
- More difficult to track upstream changes

**Rejected because:** Not compatible with goal to contribute to upstream

## Files to Modify (When Implementing)

### Primary Changes

**File:** `watch-faces/complication/tomato_face.c`

**Changes:**
1. Add `button_beep()` helper function (after line 50)
2. Replace ~8-10 hardcoded beep calls with `button_beep()`
3. Keep alarm/notification sounds unchanged

**Estimated effort:** 15-30 minutes implementation + testing

### Future Changes (When rtc32 Merges)

**File:** `movement_config.h` or `movement.h`

**Changes:**
1. Add `#define MOVEMENT_BUTTON_VOLUME_AVAILABLE`

**Estimated effort:** 1 minute

## Button Event Patterns: DOWN vs UP

### Overview

Second-movement supports both DOWN (on press) and UP (on release) button events. The choice between them affects UI responsiveness and is **independent of rtc32 architecture** - this works on current main branch.

### Event Timeline

```
Button Press → DOWN event fires immediately
     ↓
Hold for ~2s → LONG_PRESS event fires (while still held)
     ↓
Release → UP event fires (or LONG_UP if long press occurred)
```

**Key insight:** Long press detection is independent - you can handle both DOWN and LONG_PRESS without conflicts.

### Trade-offs

| Aspect | DOWN (Snappy) | UP (Deliberate) |
|--------|---------------|-----------------|
| **Response time** | Instant | Delayed until release |
| **Feel** | Modern, responsive | Traditional, intentional |
| **Accidental triggers** | More likely | Less likely |
| **Cancel action** | Must use long press | Can release elsewhere |
| **Best for** | Increments, cycles, toggles | Commits, starts, confirmations |

### Tomato Face Implementation (Current)

**Already uses DOWN:**
- ✅ `EVENT_LIGHT_BUTTON_DOWN` - LED illumination (instant feedback)
- ✅ `EVENT_ALARM_BUTTON_DOWN` - Settings increment (snappy adjustments) **[IMPLEMENTED]**

**Uses UP (intentional):**
- `EVENT_ALARM_BUTTON_UP` - Start/pause timer (deliberate action)

**Uses LONG_PRESS:**
- `EVENT_LIGHT_LONG_PRESS` - Reset count
- `EVENT_ALARM_LONG_PRESS` - Toggle autorun or enter settings

### Optional Future Improvements (Not Implemented)

#### Phase Cycling on DOWN

**Current:** Phase cycling uses `EVENT_LIGHT_BUTTON_UP` (line ~443)

**Could change to:**
```c
case EVENT_LIGHT_BUTTON_DOWN:
    if (!state->is_started) {
        _cycle_phase_forward(state);
```

**Impact:** Phase selection feels more responsive

**Trade-off:** Slightly more prone to accidental phase changes

**Recommendation:** Current implementation (UP) is fine - phase selection is deliberate enough that the delay is acceptable

### Why Settings Increment Uses DOWN

**Rationale:**
- Users often click rapidly through duration values (25→26→27→etc)
- Instant feedback makes adjustment feel more natural
- Quick ticks (ALARM long hold) still works perfectly for rapid changes
- No conflict with long press functionality

**Implementation:** Changed from `EVENT_ALARM_BUTTON_UP` to `EVENT_ALARM_BUTTON_DOWN` in `_handle_settings_mode()`

## Notes and Considerations

### Limitations of Forward Compatibility Approach
- Cannot use rtc32's `movement_get_utc_timestamp()` - requires different RTC architecture
- Cannot benefit from cached conversions - architecture-dependent optimization
- These are low-impact optimizations anyway - button beep helper is the main user-facing benefit

### When to Revisit This Plan
- If Alessandro's rtc32 branch adds more user-facing APIs worth adopting
- If upstream (Joey) requests changes before accepting tomato_face PR
- If you decide you want rtc32-specific optimizations sooner (would need multi-branch approach)
- If rtc32 merge timeline becomes clear and imminent

### Testing Strategy
1. **Test on current main:** Ensure fallback path works correctly
2. **Optional: Test on rtc32:** Create temporary branch to verify rtc32 path works
3. **Integration test:** Verify button beeps respect settings_face preferences (when rtc32 merged)

## Current Tomato Face Features

For reference, the current implementation includes:
- User-configurable settings mode (work/break/long break durations, cycle count)
- Autorun mode for automatic session transitions
- Low energy mode optimization with minimal display
- Hardware colon blinking on custom LCD
- Paused display optimization
- Background task support for timer continuation
- Complete documentation in `documents/tomato_face.md`

All these features work perfectly on current main and will continue to work with no changes needed when rtc32 merges.

## Conclusion

The forward-compatibility strategy provides the best balance of:
- **Simplicity:** Single branch, minimal maintenance
- **Functionality:** Works on main today
- **Future-proof:** Ready for rtc32 benefits automatically
- **Contribution-ready:** Can PR to upstream anytime

When you're ready to implement the button beep helper, refer to Step 1 and Step 2 above. Until then, the current tomato_face implementation is complete and production-ready.
