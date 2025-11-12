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

#include <stdlib.h>
#include <string.h>
#include "tomato_face.h"
#include "watch.h"
#include "watch_utility.h"

// Flash storage configuration
#define TOMATO_STORAGE_ROW 0
#define TOMATO_STORAGE_MAGIC 0x544F4D41  // "TOMA" in hex

typedef struct {
    uint32_t magic;
    tomato_preset_t presets[2];
    uint8_t active_preset;
    uint8_t padding[3];  // Align to 16 bytes
} tomato_storage_t;

// Duration settings now stored in preset structures

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
static void _tomato_save_presets(tomato_state_t *state);
static void _tomato_load_presets(tomato_state_t *state);

static void _tomato_save_presets(tomato_state_t *state) {
    tomato_storage_t storage;
    storage.magic = TOMATO_STORAGE_MAGIC;
    storage.presets[0] = state->presets[0];
    storage.presets[1] = state->presets[1];
    storage.active_preset = state->active_preset;
    storage.padding[0] = storage.padding[1] = storage.padding[2] = 0xFF;

    watch_storage_erase(TOMATO_STORAGE_ROW);
    watch_storage_write(TOMATO_STORAGE_ROW, 0, (uint8_t *)&storage, sizeof(storage));
    watch_storage_sync();
}

static void _tomato_load_presets(tomato_state_t *state) {
    tomato_storage_t storage;
    watch_storage_read(TOMATO_STORAGE_ROW, 0, (uint8_t *)&storage, sizeof(storage));

    if (storage.magic == TOMATO_STORAGE_MAGIC) {
        // Valid saved presets found
        state->presets[0] = storage.presets[0];
        state->presets[1] = storage.presets[1];
        state->active_preset = storage.active_preset;
    } else {
        // No saved presets, use defaults
        // Preset 1: Standard Pomodoro (25/5/20, 4 rounds)
        state->presets[0].work_min = 25;
        state->presets[0].break_min = 5;
        state->presets[0].long_break_min = 20;
        state->presets[0].rounds = 4;

        // Preset 2: Long session (50/10/30, 4 rounds)
        state->presets[1].work_min = 50;
        state->presets[1].break_min = 10;
        state->presets[1].long_break_min = 30;
        state->presets[1].rounds = 4;

        state->active_preset = 0;

        // Save defaults to flash
        _tomato_save_presets(state);
    }
}

static uint8_t get_length(tomato_state_t *state) {
    tomato_preset_t *preset = &state->presets[state->active_preset];
    switch (state->phase) {
        case tomato_break:
            return preset->break_min;
        case tomato_long_break:
            return preset->long_break_min;
        default:
            return preset->work_min;
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
    tomato_preset_t *preset = &state->presets[state->active_preset];
    int result = num % preset->rounds;
    return result > 0 ? result : preset->rounds;
}

static void _increment_setting_value(tomato_state_t *state) {
    tomato_preset_t *preset = &state->presets[state->active_preset];
    switch (state->current_setting) {
        case setting_work_min:
            preset->work_min++;
            if (preset->work_min > 60) preset->work_min = 1;
            _tomato_save_presets(state);
            break;
        case setting_short_break:
            preset->break_min++;
            if (preset->break_min > 30) preset->break_min = 1;
            _tomato_save_presets(state);
            break;
        case setting_long_break:
            preset->long_break_min++;
            if (preset->long_break_min > 60) preset->long_break_min = 1;
            _tomato_save_presets(state);
            break;
        case setting_cycles:
            preset->rounds++;
            if (preset->rounds > 10) preset->rounds = 1;
            _tomato_save_presets(state);
            break;
        case setting_reset_count:
            state->count = 0;
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
            state->phase = tomato_summary;
            watch_buzzer_play_sequence((int8_t *)_sound_seq_low_beep, NULL);
            break;
        case tomato_summary:
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
        case EVENT_LIGHT_LONG_PRESS:
            // Exit settings mode immediately (without cycling through all settings)
            state->mode = tomato_mode_normal;
            movement_request_tick_frequency(1);
            watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
            _tomato_draw(state);
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
    const char *name_custom, *name_classic;
    uint8_t value;
    tomato_preset_t *preset = &state->presets[state->active_preset];

    // Get current setting name and value
    switch (state->current_setting) {
        case setting_work_min:
            name_custom = "FOCUS";
            name_classic = "St";
            value = preset->work_min;
            break;
        case setting_short_break:
            name_custom = "break";
            name_classic = "br";
            value = preset->break_min;
            break;
        case setting_long_break:
            name_custom = "LnBrk";
            name_classic = "Br";
            value = preset->long_break_min;
            break;
        case setting_cycles:
            name_custom = "CYCLE";
            name_classic = "CL";
            value = preset->rounds;
            break;
        case setting_reset_count:
            name_custom = "COUNT";
            name_classic = "Ct";
            value = state->count;
            break;
        default:
            return;
    }

    watch_clear_display();

    watch_display_text_with_fallback(WATCH_POSITION_TOP, name_custom, name_classic);

    // Blink value on odd subseconds (unless quick ticks running or count reset)
    if (subsecond % 2 && !state->quick_ticks_running && state->current_setting != setting_reset_count) {
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
    } else {
        sprintf(buf, "  %2d", value);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

static void _tomato_draw(tomato_state_t *state) {
    char buf[16];
    tomato_preset_t *preset = &state->presets[state->active_preset];

    // Preset indicators: simple numeric (1, 2)
    uint8_t preset_num = state->active_preset + 1;

    if (!state->is_visible) return;

    watch_clear_display();

    if (state->is_started) {
        // Timer is running or paused - show current session
        uint32_t delta;
        div_t result;
        uint8_t min, sec;

        if (state->is_paused) {
            result = div(state->remainder, 60);
            min = result.quot;
            sec = result.rem;
        } else {
            delta = state->target_ts - state->now_ts;
            result = div(delta, 60);
            min = result.quot;
            sec = result.rem;
        }

        // Show phase with preset number - different lengths for custom vs classic
        // Top left has 3 chars when using top right for rounds (OOO oo layout = 3+2)
        char phase_custom[5], phase_classic[4];
        char roman = (preset_num == 1) ? '{' : '|';  // '{' = I, '|' = II
        switch(state->phase) {
            case tomato_focus:
                sprintf(phase_custom, "FO%d", preset_num);  // 3 chars for custom
                sprintf(phase_classic, "F%c", roman);  // Roman numerals for classic
                break;
            case tomato_break:
                sprintf(phase_custom, "br%d", preset_num);  // lowercase for short break
                sprintf(phase_classic, "b%c", roman);
                break;
            case tomato_long_break:
                sprintf(phase_custom, "LB%d", preset_num);  // LB = Long Break
                sprintf(phase_classic, "B%c", roman);
                break;
        }

        watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, phase_custom, phase_classic);

        // Top right: show current round
        char round_buf[3];
        sprintf(round_buf, "%2d", _rounds(state, state->count));
        watch_display_text(WATCH_POSITION_TOP_RIGHT, round_buf);

        // Bottom: minutes, seconds, count
        // Custom LCD has decimal segment (same position as colon) for min:sec separator
        sprintf(buf, "%2d%02d%2d", min, sec, state->count);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);

        // Show separator: colon for classic LCD, decimal for custom LCD
        if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
            watch_set_colon();
        } else {
            watch_set_decimal_if_available();
        }
    } else {
        // Timer is stopped - show based on current phase
        // Top left has 3 chars when using top right for rounds (OOO oo layout = 3+2)
        char top_custom[5], top_classic[4];
        char roman = (preset_num == 1) ? '{' : '|';  // '{' = I, '|' = II

        if (state->phase == tomato_summary) {
            // Summary view: show "TO1"/"T1" indicator with all three durations
            sprintf(top_custom, "TO%d", preset_num);  // Tomato indicator for custom
            sprintf(top_classic, "T%c", roman);       // Tomato indicator for classic

            // Top right: cycles
            char cycles_buf[3];
            sprintf(cycles_buf, "%2d", preset->rounds);

            // Bottom: all three durations (work/break/long break: 250520 = 25min/5min/20min)
            sprintf(buf, "%02d%02d%02d", preset->work_min, preset->break_min, preset->long_break_min);

            printf("SUMMARY VIEW - LCD type: %d\n  TOP_LEFT: custom='%s' classic='%s' (preset=%d, roman='%c')\n  TOP_RIGHT: '%s'\n  BOTTOM: '%s'\n",
                   watch_get_lcd_type(), top_custom, top_classic, preset_num, roman, cycles_buf, buf);

            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, top_custom, top_classic);
            watch_display_text(WATCH_POSITION_TOP_RIGHT, cycles_buf);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
        } else {
            // Individual phase view: show specific phase details
            // Build full display string for both custom and classic
            // Format: positions 0-9 are sequential, position 10 is the 3rd top-left char (custom only)
            // Layout: [0][1] [2][3] [4][5][6][7][8][9] [10]
            //         TopL   TopR   Bottom (6 chars)   3rdTopL

            char full_custom[13];  // 12 chars + null (worst case)
            char full_classic[11]; // 10 chars + null

            uint8_t duration = get_length(state);
            char phase_char1, phase_char2;      // First 2 chars of phase for custom LCD
            char phase_char3_custom;            // 3rd char for custom LCD
            char phase_char_classic;            // Single char for classic LCD

            switch(state->phase) {
                case tomato_focus:
                    phase_char1 = 'F'; phase_char2 = 'O'; phase_char3_custom = '0' + preset_num;
                    phase_char_classic = 'F';
                    break;
                case tomato_break:
                    phase_char1 = 'b'; phase_char2 = 'r'; phase_char3_custom = '0' + preset_num;
                    phase_char_classic = 'b';
                    break;
                case tomato_long_break:
                    phase_char1 = 'L'; phase_char2 = 'B'; phase_char3_custom = '0' + preset_num;
                    phase_char_classic = 'B';  // Use 'B' not 'L' for long break on classic
                    break;
                case tomato_summary:
                    // Should never reach here (handled above), but for safety
                    phase_char1 = 'T'; phase_char2 = 'O'; phase_char3_custom = '0' + preset_num;
                    phase_char_classic = 'T';
                    break;
            }

            // Build custom LCD string (up to 12 chars + null): "LB 4  20  01"
            //                                                   [0][1][2][3][4][5][6][7][8][9][10]
            // Format: TopL(2) + TopR(2) + Bottom(6) + TopL3rd(1) = 11 chars
            sprintf(full_custom, "%c%c%2d  %2d %d%c",
                    phase_char1, phase_char2, preset->rounds,
                    duration, state->count, phase_char3_custom);

            // Build classic LCD string (10 chars + null): "B{ 4  20 0"
            //                                             [0][1][2][3][4][5][6][7][8][9]
            // Use roman numerals for preset, dedicated char for classic
            sprintf(full_classic, "%c%c%2d  %2d %d",
                    phase_char_classic, roman, preset->rounds,
                    duration, state->count);

            printf("PHASE VIEW - LCD type: %d\n  FULL: custom='%s' classic='%s'\n",
                   watch_get_lcd_type(), full_custom, full_classic);

            watch_display_text_with_fallback(WATCH_POSITION_FULL, full_custom, full_classic);
        }

        watch_clear_decimal_if_available();  // Clear decimal when stopped
        watch_clear_colon();  // Clear colon during phase preview
    }

    // Set LAP indicator if autorun is enabled (restore after any display clearing)
    if (state->is_autorun) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_LAP);
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

    tomato_preset_t *preset = &state->presets[state->active_preset];
    if (state->phase == tomato_focus) {
        if (_rounds(state, state->count) == preset->rounds) {
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
        // Load presets from flash or initialize defaults
        _tomato_load_presets(state);
    }
}

void tomato_face_activate(void *context) {
    tomato_state_t *state = (tomato_state_t *)context;
    watch_date_time_t now;

    if (state->is_started) {
        now = movement_get_utc_date_time();
        state->now_ts = watch_utility_date_time_to_unix_time(now, 0);
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        // Start in summary view when stopped
        state->phase = tomato_summary;
    }
    if (state->is_autorun) {
        watch_set_indicator(WATCH_INDICATOR_LAP);
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
            // Enter settings mode when stopped
            if (!state->is_started) {
                state->mode = tomato_mode_setting;
                state->current_setting = 0;
                movement_request_tick_frequency(4);
                watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
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
                // Normalize phase: summary view always starts from focus
                if (state->phase == tomato_summary) {
                    state->phase = tomato_focus;
                }
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
                // Toggle preset when stopped
                state->active_preset = (state->active_preset + 1) % 2;
                state->phase = tomato_summary;  // Always show summary after preset switch
                _tomato_save_presets(state);
                watch_buzzer_play_sequence((int8_t *)_sound_seq_beep, NULL);
                _tomato_draw(state);
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
                watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
                if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
                    watch_start_sleep_animation(500);
                }

                // Update time once per minute
                watch_date_time_t now = movement_get_utc_date_time();
                state->now_ts = watch_utility_date_time_to_unix_time(now, 0);

                uint32_t delta = state->target_ts - state->now_ts;
                div_t result = div(delta, 60);
                uint8_t min = result.quot;

                // Same preset indicators as main display
                // Top left has 3 chars when using top right for rounds (OOO oo layout = 3+2)
                uint8_t preset_num = state->active_preset + 1;
                char phase_custom[5], phase_classic[4];
                char roman = (preset_num == 1) ? '{' : '|';  // '{' = I, '|' = II

                switch(state->phase) {
                    case tomato_focus:
                        sprintf(phase_custom, "FO%d", preset_num);
                        sprintf(phase_classic, "F%c", roman);  // Roman numerals for classic
                        break;
                    case tomato_break:
                        sprintf(phase_custom, "br%d", preset_num);  // lowercase for short break
                        sprintf(phase_classic, "b%c", roman);
                        break;
                    case tomato_long_break:
                        sprintf(phase_custom, "LB%d", preset_num);  // LB = Long Break
                        sprintf(phase_classic, "B%c", roman);
                        break;
                }

                watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, phase_custom, phase_classic);
                watch_display_text(WATCH_POSITION_BOTTOM, "      ");
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

