/*
 * MIT License
 *
 * Copyright (c) 2022 Wesley Ellis
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFtomato_ringEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "tomato_face.h"
#include "watch.h"
#include "watch_utility.h"

// Duration settings now stored in state struct

static const int8_t _sound_seq_alarm[] = {BUZZER_NOTE_C8, 3, BUZZER_NOTE_REST, 3, -2, 2, BUZZER_NOTE_C8, 5, BUZZER_NOTE_REST, 25, 0};
static const int8_t _sound_seq_beep[] = {BUZZER_NOTE_G8, 2, 0};
static const int8_t _sound_seq_low_beep[] = {BUZZER_NOTE_G7, 2, 0};
static const int8_t _sound_seq_double_beep[] = {
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_C8, 3,
    0
    };
static const int8_t _sound_seq_pause[] = {
    BUZZER_NOTE_C8, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_E8, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_C8, 3,
    0
    };

// Forward declarations
static void _tomato_draw(tomato_state_t *state);
static void _tomato_draw_setting(tomato_state_t *state, uint8_t subsecond);

static uint8_t get_length(tomato_state_t *state) {
    switch (state->phase) {
        case tomato_break:
            return state->break_min;
        case tomato_long_break:
            return state->long_break_min;
        default:
            return state->work_min;
    }
}

static void _tomato_start(tomato_state_t *state, bool with_beep) {
    watch_date_time_t now;
    int8_t length;
    watch_date_time_t target_dt;

    now = movement_get_utc_date_time();
    length = (int8_t) get_length(state);

    if (state->phase == tomato_focus) {
        state->count++;
    }
    state->is_started = true;
    state->now_ts = watch_utility_date_time_to_unix_time(now, 0);
    state->target_ts = watch_utility_offset_timestamp(state->now_ts, 0, length, 0);
    target_dt = watch_utility_date_time_from_unix_time(state->target_ts, 0);
    movement_schedule_background_task_for_face(state->watch_face_index, target_dt);
    watch_set_indicator(WATCH_INDICATOR_BELL);
    watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
    if (with_beep) watch_buzzer_play_sequence((int8_t *)_sound_seq_double_beep, NULL);
}

static void _tomato_pause(tomato_state_t *state) {
    state->is_paused = true;
    state->remainder = state->target_ts - state->now_ts;
    // printf("pause, remainder: %d\n", state->remainder);
    movement_cancel_background_task_for_face(state->watch_face_index);
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_stop_blink();
    watch_buzzer_play_sequence((int8_t *)_sound_seq_pause, NULL);
}

static void _tomato_resume(tomato_state_t *state) {
    watch_date_time_t now;
    div_t result;
    watch_date_time_t target_dt;

    // printf("unpause, remainder: %d\n", state->remainder);
    now = movement_get_utc_date_time();
    state->is_paused = false;
    state->now_ts = watch_utility_date_time_to_unix_time(now, 0);
    result = div(state->remainder, 60);
    state->target_ts = watch_utility_offset_timestamp(state->now_ts, 0, result.quot, result.rem);
    // printf("target_ts: %d\n", state->target_ts);
    // printf("now_ts: %d\n", state->now_ts);
    target_dt = watch_utility_date_time_from_unix_time(state->target_ts, 0);
    movement_schedule_background_task_for_face(state->watch_face_index, target_dt);
    watch_set_indicator(WATCH_INDICATOR_BELL);
    watch_buzzer_play_sequence((int8_t *)_sound_seq_pause, NULL);
}

static int _rounds(tomato_state_t *state, int num) {
    if (num == 0) return 0;
    int result = num % state->rounds;
    return result > 0 ? result : state->rounds;
}

static void _increment_setting_value(tomato_state_t *state) {
    switch (state->current_setting) {
        case setting_work_min:
            state->work_min++;
            if (state->work_min > 60) state->work_min = 1;
            break;
        case setting_short_break:
            state->break_min++;
            if (state->break_min > 30) state->break_min = 1;
            break;
        case setting_long_break:
            state->long_break_min++;
            if (state->long_break_min > 60) state->long_break_min = 1;
            break;
        case setting_cycles:
            state->rounds++;
            if (state->rounds > 10) state->rounds = 1;
            break;
        default:
            break;
    }
}

static void _cycle_phase_forward(tomato_state_t *state) {
    switch(state->phase) {
        case tomato_focus:
            state->phase = tomato_break;
            watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
            break;
        case tomato_break:
            state->phase = tomato_long_break;
            watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
            break;
        case tomato_long_break:
            state->phase = tomato_focus;
            watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
            break;
    }
}

static bool _handle_settings_mode(movement_event_t event, tomato_state_t *state) {
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _tomato_draw_setting(state, event.subsecond);
            // Handle quick ticks
            if (state->quick_ticks_running && HAL_GPIO_BTN_ALARM_read()) {
                _increment_setting_value(state);
            } else if (state->quick_ticks_running) {
                // Button released, abort quick ticks
                state->quick_ticks_running = false;
                movement_request_tick_frequency(4);
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // Cycle to next setting
            state->current_setting++;
            if (state->current_setting >= SETTING_COUNT) {
                // Exit settings mode
                state->mode = tomato_mode_normal;
                movement_request_tick_frequency(1);
                watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
                _tomato_draw(state);
            } else {
                watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
                _tomato_draw_setting(state, event.subsecond);
            }
            break;
        case EVENT_ALARM_BUTTON_DOWN:
            _increment_setting_value(state);
            watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
            _tomato_draw_setting(state, event.subsecond);
            break;
        case EVENT_ALARM_LONG_PRESS:
            // Start quick ticks
            state->quick_ticks_running = true;
            movement_request_tick_frequency(8);
            break;
        case EVENT_ALARM_LONG_UP:
            // Stop quick ticks
            if (state->quick_ticks_running) {
                state->quick_ticks_running = false;
                movement_request_tick_frequency(4);
                watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
            }
            break;
        case EVENT_TIMEOUT:
            // Exit settings on timeout
            state->mode = tomato_mode_normal;
            movement_request_tick_frequency(1);
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

static void _tomato_draw_setting(tomato_state_t *state, uint8_t subsecond) {
    char buf[16];
    const char *name;
    uint8_t value;

    // Get current setting name and value
    switch (state->current_setting) {
        case setting_work_min:
            name = "St";
            value = state->work_min;
            break;
        case setting_short_break:
            name = "br";
            value = state->break_min;
            break;
        case setting_long_break:
            name = "Br";
            value = state->long_break_min;
            break;
        case setting_cycles:
            name = "Cy";
            value = state->rounds;
            break;
        default:
            return;
    }

    watch_clear_display();

    // Blink value on odd subseconds (unless quick ticks running)
    if (subsecond % 2 && !state->quick_ticks_running) {
        sprintf(buf, "%s      ", name);
    } else {
        sprintf(buf, "%s  %2d", name, value);
    }

    watch_display_text(WATCH_POSITION_FULL, buf);
}

static void _tomato_draw(tomato_state_t *state) {
    char buf[16];

    uint32_t delta;
    div_t result;
    uint8_t min = 0;
    uint8_t sec = 0;

    // char phase;
    // if (state->phase == tomato_break) {
    //     phase = 'b';
    // } else {
    //     phase = 'f';
    // }

    if (state->is_started) {
        if (state->is_paused) {
            result = div(state->remainder, 60);
            min = result.quot;
            sec = result.rem;
            // Blinking indicator now handled in EVENT_TICK
            // printf("remainder: %d\n", state->remainder);
            // printf("min: %d\n", min);
            // printf("sec: %d\n", sec);
        } else {
            delta = state->target_ts - state->now_ts;
            result = div(delta, 60);
            min = result.quot;
            sec = result.rem;
            // printf("target_ts: %d\n", state->target_ts);
            // printf("now_ts: %d\n", state->now_ts);
            // printf("delta: %d\n", delta);
            // printf("min: %d\n", min);
            // printf("sec: %d\n", sec);
        }
    } else {
        min = get_length(state);
        sec = 0;
    }

    char title[3];
    // printf("now_ts: %d\n", state->now_ts);
    // uint8_t rest = state->now_ts%2;
    // printf("rest: %d\n", rest);
    // if (state->is_paused && rest == 1) {
    //     strcpy(title, "PA");
    // } else {
    //     switch(state->phase) {
    //         case tomato_focus:
    //             strcpy(title, "FO");
    //             break;
    //         case tomato_break:
    //             strcpy(title, "BR");
    //             break;
    //         case tomato_long_break:
    //             strcpy(title, "LB");
    //             break;
    //     }
    // }
    switch(state->phase) {
            case tomato_focus:
                strcpy(title, "FO");
                break;
            case tomato_break:
                strcpy(title, "br");
                break;
            case tomato_long_break:
                strcpy(title, "Br");
                break;
        }
    
    if (state->is_visible) {
        sprintf(buf, "%2s%2d%2d%02d%2d", title, state->count, min, sec, _rounds(state, state->count));
        // printf("%s\n", buf);
        watch_display_text(WATCH_POSITION_FULL, buf);
    }
}

static void _tomato_reset_state(tomato_state_t *state) {
    state->is_started = false;
    state->is_paused = false;
    movement_cancel_background_task_for_face(state->watch_face_index);
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_stop_blink();
}

static void tomato_ring(tomato_state_t *state) {
    // movement_play_signal();
    watch_buzzer_play_sequence((int8_t *)_sound_seq_alarm, NULL);

    if (state->phase == tomato_focus) {
        if (_rounds(state, state->count) == state->rounds) {
            state->phase = tomato_long_break;
        } else {
            state->phase = tomato_break;
        }
    } else {
        state->phase = tomato_focus;
    }

    _tomato_reset_state(state);

    if (state->is_autorun) {
         _tomato_start(state, false);
    }
}

static void _set_autorun(tomato_state_t *state, bool autorun) {
    state->is_autorun = autorun;
    if (autorun) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
    }
}

void tomato_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tomato_state_t));
        tomato_state_t *state = (tomato_state_t*)*context_ptr;
        memset(*context_ptr, 0, sizeof(tomato_state_t));
        state->watch_face_index = watch_face_index;
        state->is_started = false;
        state->is_paused = false;
        state->phase= tomato_focus;
        state->count = 0;
        state->is_visible = true;
        state->is_autorun = false;
        // Initialize settings mode
        state->mode = tomato_mode_normal;
        state->current_setting = 0;
        state->quick_ticks_running = false;
        // Initialize default durations (standard Pomodoro values)
        state->work_min = 25;
        state->break_min = 5;
        state->long_break_min = 20;
        state->rounds = 4;
    }
}

void tomato_face_activate(void *context) {
    tomato_state_t *state = (tomato_state_t *)context;
    watch_date_time_t now;

    if (state->is_started) {
        now = movement_get_utc_date_time();
        state->now_ts = watch_utility_date_time_to_unix_time(now, 0);
        watch_set_indicator(WATCH_INDICATOR_BELL);
    }
    watch_set_colon();
    state->is_visible = true;
}

bool tomato_face_loop(movement_event_t event, void *context) {
    tomato_state_t *state = (tomato_state_t *)context;

    // Handle settings mode separately
    if (state->mode == tomato_mode_setting) {
        return _handle_settings_mode(event, state);
    }

    // Normal mode handlers
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _tomato_draw(state);
            break;
        case EVENT_TICK:
            if (state->is_paused) {
                // Only update blinking indicator, skip full redraw
                uint8_t rest = state->now_ts % 2;
                if (rest == 1) {
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
                } else {
                    watch_set_indicator(WATCH_INDICATOR_BELL);
                }
                state->now_ts++;
            } else {
                state->now_ts++;
                _tomato_draw(state);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // Cycle phases when stopped
            if (!state->is_started) {
                _cycle_phase_forward(state);
                _tomato_draw(state);
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            // Reset count when stopped
            if (!state->is_started) {
                state->count = 0;
                watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
                _tomato_draw(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->is_started) {
                // Pause/resume when running
                if (state->is_paused) {
                    _tomato_resume(state);
                } else {
                    _tomato_pause(state);
                }
                _tomato_draw(state);
            } else {
                // Start timer when stopped
                _tomato_start(state, true);
                _tomato_draw(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->is_started) {
                // Toggle autorun or reset when running/paused
                if (state->is_paused) {
                    if (state->phase == tomato_focus) {
                        state->count--;
                    }
                    _tomato_reset_state(state);
                } else {
                    if (state->is_autorun) {
                        _set_autorun(state, false);
                    } else {
                        _set_autorun(state, true);
                    }
                }
                watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
                _tomato_draw(state);
            } else {
                // Enter settings mode when stopped
                state->mode = tomato_mode_setting;
                state->current_setting = 0;
                movement_request_tick_frequency(4);
                watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
                // Settings display will be handled below
            }
            break;
        case EVENT_BACKGROUND_TASK:
            tomato_ring(state);
            _tomato_draw(state);
            break;
        case EVENT_TIMEOUT:
            if (!state->is_started || state->is_paused) {
                movement_move_to_face(0);
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (state->is_started && !state->is_paused) {
                // Running - show minimal display with animation
                if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
                    watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
                } else {
                    watch_start_sleep_animation(500);
                }

                // Update time once per minute
                watch_date_time_t now = movement_get_utc_date_time();
                state->now_ts = watch_utility_date_time_to_unix_time(now, 0);

                uint32_t delta = state->target_ts - state->now_ts;
                div_t result = div(delta, 60);
                uint8_t min = result.quot;

                char buf[16];
                switch(state->phase) {
                    case tomato_focus:
                        sprintf(buf, "FO  %2d--", min);
                        break;
                    case tomato_break:
                        sprintf(buf, "br  %2d--", min);
                        break;
                    case tomato_long_break:
                        sprintf(buf, "Br  %2d--", min);
                        break;
                }
                watch_display_text(WATCH_POSITION_FULL, buf);
            } else {
                // Paused or stopped - navigate away for consistency
                movement_move_to_face(0);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void tomato_face_resign(void *context) {
    tomato_state_t *state = (tomato_state_t *)context;
    state->is_visible = false;
}

