/*
 * MIT License
 *
 * Copyright (c) 2022 Wesley Ellis
 * Copyright (c) 2025 Oleksandr Kostenko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TOMATO_FACE_H_
#define TOMATO_FACE_H_

/*
 * TOMATO TIMER face
 *
 * Pomodoro Technique timer for Sensor Watch implementing work/break cycles.
 *  https://en.wikipedia.org/wiki/Pomodoro_Technique
 *
 * Original concept and implementation by Wesley Ellis (2022)
 * Significantly enhanced for second-movement by Oleksandr Kostenko (2025):
 *  - Pause/resume, long breaks, autorun mode
 *  - User-configurable durations via settings mode
 *  - Low energy mode support
 *
 * Features:
 *  - Focus sessions (default 25 min), short breaks (5 min), long breaks (20 min)
 *  - Configurable cycle count before long break (default 4)
 *  - Pause/resume with ALARM button
 *  - Autorun mode for automatic session transitions
 *  - Background task support (timer runs when on other faces)
 *  - Settings mode: ALARM long press when stopped
 *
 * See documents/tomato_face.md for complete documentation.
 */

#include "movement.h"

typedef enum {
    tomato_focus,
    tomato_break,
    tomato_long_break,
    tomato_summary      // View-only state for preset overview (stopped state only)
} tomato_phase;

typedef enum {
    tomato_mode_normal,
    tomato_mode_setting
} tomato_mode_t;

typedef enum {
    setting_work_min,
    setting_short_break,
    setting_long_break,
    setting_cycles,
    setting_reset_count,
    SETTING_COUNT
} tomato_setting_t;

typedef struct {
    uint8_t work_min;
    uint8_t break_min;
    uint8_t long_break_min;
    uint8_t rounds;
} tomato_preset_t;

typedef struct {
    uint32_t target_ts;
    uint32_t now_ts;
    // how many seconds remainded after pause
    uint32_t remainder;
    tomato_phase phase;
    // counts focused phases
    uint8_t count;
    uint8_t watch_face_index;
    // settings mode
    tomato_mode_t mode;
    tomato_setting_t current_setting;
    bool quick_ticks_running;
    // preset system
    tomato_preset_t presets[2];
    uint8_t active_preset;  // 0 or 1
    // flags
    bool is_visible;
    bool is_started;
    bool is_paused;
    bool is_autorun;
} tomato_state_t;

void tomato_face_setup(uint8_t watch_face_index, void ** context_ptr);
void tomato_face_activate(void *context);
bool tomato_face_loop(movement_event_t event, void *context);
void tomato_face_resign(void *context);

#define tomato_face ((const watch_face_t){ \
    tomato_face_setup, \
    tomato_face_activate, \
    tomato_face_loop, \
    tomato_face_resign, \
    NULL, \
})

#endif // TOMATO_FACE_H_

