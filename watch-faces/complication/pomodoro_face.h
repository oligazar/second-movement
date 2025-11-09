/*
 * MIT License
 *
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

#ifndef POMODORO_FACE_H_
#define POMODORO_FACE_H_

/*
 * POMODORO TIMER face
 *
 * A productivity timer based on the Pomodoro Technique:
 *   - 25-minute work sessions
 *   - 5-minute short breaks
 *   - 15-minute long breaks (after 4 work sessions)
 *
 * Usage:
 *   - ALARM button: Start/Pause timer
 *   - LIGHT button: Skip to next session
 *   - The watch will beep when a session completes
 *   - Display shows: session type indicator + time remaining
 *     - "PO" = Pomodoro work session
 *     - "br" = Short break
 *     - "Br" = Long break
 */

#include "movement.h"

typedef enum {
    pomodoro_stopped,
    pomodoro_running,
    pomodoro_paused,
    pomodoro_completed
} pomodoro_mode_t;

typedef enum {
    session_work,
    session_short_break,
    session_long_break
} pomodoro_session_t;

typedef struct {
    uint32_t target_ts;
    uint32_t now_ts;
    uint16_t minutes_left;
    uint8_t seconds_left;
    uint8_t completed_pomodoros;
    pomodoro_mode_t mode;
    pomodoro_session_t session_type;
    uint8_t watch_face_index;
    bool beep_started;
} pomodoro_state_t;

void pomodoro_face_setup(uint8_t watch_face_index, void ** context_ptr);
void pomodoro_face_activate(void *context);
bool pomodoro_face_loop(movement_event_t event, void *context);
void pomodoro_face_resign(void *context);

#define pomodoro_face ((const watch_face_t){ \
    pomodoro_face_setup, \
    pomodoro_face_activate, \
    pomodoro_face_loop, \
    pomodoro_face_resign, \
    NULL, \
})

#endif // POMODORO_FACE_H_
