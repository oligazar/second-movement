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

#include <stdlib.h>
#include <string.h>
#include "pomodoro_face.h"
#include "watch.h"
#include "watch_utility.h"

#define POMODORO_WORK_MINUTES 25
#define POMODORO_SHORT_BREAK_MINUTES 5
#define POMODORO_LONG_BREAK_MINUTES 15
#define POMODOROS_UNTIL_LONG_BREAK 4

static void _pomodoro_start_session(pomodoro_state_t *state, pomodoro_session_t session_type) {
    state->session_type = session_type;
    state->mode = pomodoro_stopped;
    state->beep_started = false;

    uint8_t duration;
    switch (session_type) {
        case session_work:
            duration = POMODORO_WORK_MINUTES;
            break;
        case session_short_break:
            duration = POMODORO_SHORT_BREAK_MINUTES;
            break;
        case session_long_break:
            duration = POMODORO_LONG_BREAK_MINUTES;
            break;
        default:
            duration = POMODORO_WORK_MINUTES;
            break;
    }

    state->minutes_left = duration;
    state->seconds_left = 0;
}

static void _pomodoro_next_session(pomodoro_state_t *state) {
    if (state->session_type == session_work) {
        state->completed_pomodoros++;

        if (state->completed_pomodoros % POMODOROS_UNTIL_LONG_BREAK == 0) {
            _pomodoro_start_session(state, session_long_break);
        } else {
            _pomodoro_start_session(state, session_short_break);
        }
    } else {
        _pomodoro_start_session(state, session_work);
    }
}

static void _pomodoro_update_display(pomodoro_state_t *state) {
    char buf[16];

    // Clear display
    watch_clear_display();

    // Display session type indicator and time remaining
    if (state->session_type == session_work) {
        sprintf(buf, "PO  %2d%02d", state->minutes_left, state->seconds_left);
    } else if (state->session_type == session_short_break) {
        sprintf(buf, "br  %2d%02d", state->minutes_left, state->seconds_left);
    } else {
        sprintf(buf, "Br  %2d%02d", state->minutes_left, state->seconds_left);
    }

    watch_display_text(WATCH_POSITION_FULL, buf);

    // Show indicator if running
    if (state->mode == pomodoro_running) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }

    // Show lap indicator for completed pomodoros
    if (state->completed_pomodoros > 0) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    }
}

void pomodoro_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(pomodoro_state_t));
        memset(*context_ptr, 0, sizeof(pomodoro_state_t));
        pomodoro_state_t *state = (pomodoro_state_t *)*context_ptr;
        state->watch_face_index = watch_face_index;
        _pomodoro_start_session(state, session_work);
    }
}

void pomodoro_face_activate(void *context) {
    pomodoro_state_t *state = (pomodoro_state_t *)context;
    _pomodoro_update_display(state);
}

bool pomodoro_face_loop(movement_event_t event, void *context) {
    pomodoro_state_t *state = (pomodoro_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _pomodoro_update_display(state);
            break;

        case EVENT_TICK:
            if (state->mode == pomodoro_running) {
                state->now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);

                if (state->now_ts >= state->target_ts) {
                    // Timer completed
                    state->mode = pomodoro_completed;
                    state->minutes_left = 0;
                    state->seconds_left = 0;

                    if (!state->beep_started) {
                        movement_play_alarm();
                        state->beep_started = true;
                    }

                    _pomodoro_update_display(state);
                } else {
                    // Update display
                    uint32_t seconds_remaining = state->target_ts - state->now_ts;
                    state->minutes_left = seconds_remaining / 60;
                    state->seconds_left = seconds_remaining % 60;
                    _pomodoro_update_display(state);
                }
            }
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Start/Pause/Resume timer
            if (state->mode == pomodoro_completed) {
                // Move to next session
                _pomodoro_next_session(state);
                _pomodoro_update_display(state);
            } else if (state->mode == pomodoro_running) {
                // Pause
                state->mode = pomodoro_paused;
                state->now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
                uint32_t seconds_remaining = state->target_ts - state->now_ts;
                state->minutes_left = seconds_remaining / 60;
                state->seconds_left = seconds_remaining % 60;
                _pomodoro_update_display(state);
            } else {
                // Start/Resume
                state->mode = pomodoro_running;
                state->now_ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
                state->target_ts = state->now_ts + (state->minutes_left * 60) + state->seconds_left;

                // Schedule background task to wake us up when timer completes
                watch_date_time_t target_date_time = movement_get_local_date_time();
                uint32_t target_timestamp = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0) + (state->minutes_left * 60) + state->seconds_left;
                target_date_time = watch_utility_date_time_from_unix_time(target_timestamp, 0);
                movement_schedule_background_task_for_face(state->watch_face_index, target_date_time);

                _pomodoro_update_display(state);
            }
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Reset to work session
            state->completed_pomodoros = 0;
            _pomodoro_start_session(state, session_work);
            _pomodoro_update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Skip to next session
            if (state->mode == pomodoro_running) {
                movement_cancel_background_task_for_face(state->watch_face_index);
            }
            _pomodoro_next_session(state);
            _pomodoro_update_display(state);
            break;

        case EVENT_BACKGROUND_TASK:
            // Timer completed while in background
            movement_play_alarm();
            break;

        case EVENT_TIMEOUT:
            // Don't auto-resign if timer is running
            if (state->mode != pomodoro_running) {
                movement_move_to_face(0);
            }
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            // Just show watch face name in low energy mode
            watch_display_text(WATCH_POSITION_TOP, "Pomodoro");
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void pomodoro_face_resign(void *context) {
    pomodoro_state_t *state = (pomodoro_state_t *)context;

    // Don't cancel the background task - let it continue running
    // The timer should complete even when not on this face
    (void)state;
}
