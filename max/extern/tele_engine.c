#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tele_engine.h"
#include "tele_engine_internal.h"

#define send_update(...)                                          \
    do {                                                          \
        active_engine->watcher->callback(                         \
            active_engine, (tele_engine_update_t){ __VA_ARGS__ }, \
            active_engine->watcher->data);                        \
                                                                  \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
//
// Initialization and life-cycle.
//
////////////////////////////////////////////////////////////////////////////////

// The shared engine instance, receives callbacks during command processing.
tele_engine_t *active_engine;

// Declared in teletype.c -- the (shared) global state.
extern scene_state_t scene_state;

tele_engine_t *tele_engine_new(tele_engine_watcher watcher) {
    tele_engine_t *engine = malloc(sizeof *engine);
    if (engine != NULL) {
        tele_engine_init(engine);
        tele_init();
        engine->watcher = malloc(sizeof *engine->watcher);
        engine->watcher->callback = watcher.callback;
        engine->watcher->data = watcher.data;
    }
    return engine;
}

void tele_engine_init(tele_engine_t *engine) {
    // Sets scene state.
    tele_set_scene(DEFAULT_SCENE);

    // Snooping on global state instance.
    engine->state = &scene_state;

    for (int i = 0; i < MAX_SCENES; ++i) {
        tele_scene_init(&engine->scenes[i]);
    }
    tele_engine_set_scene(engine, DEFAULT_SCENE);
    redirect_tele_callbacks();
    active_engine = engine;
}

void tele_engine_free(tele_engine_t *engine) {
    free(engine->watcher);
    free(engine);
}

////////////////////////////////////////////////////////////////////////////////
//
// Redirectors and callbacks.
//
////////////////////////////////////////////////////////////////////////////////

void r_tele_cv(const uint8_t i, int16_t v, uint8_t s) {
    trace("cv [%i]: %i (slew: %i)\n", i, v, s);
    // 1-indexed
    send_update(.cv = i + 1, .v = true);
}

void r_tele_metro(int16_t m, int16_t m_act, uint8_t m_reset) {
    trace("metro: %i act: %i reset: %i\n", m, m_act, m_reset);
    send_update(.metro = true, .metro_act = m_act);
}

void r_tele_tr(uint8_t i, int16_t v) {
    trace("tr [%i]: %i\n", i, v);
    // 1-indexed
    send_update(.tr = i + 1);
}

void r_tele_cv_slew(uint8_t i, int16_t v) {
    trace("cv [%i] slew: %i\n", i, v);
}

void r_tele_delay(uint8_t i) {}

void r_tele_s(uint8_t i) {}

void r_tele_cv_off(uint8_t i, int16_t v) {}

void r_tele_ii(uint8_t i, int16_t d) {}

void r_tele_scene(uint8_t i) {}

void r_tele_pi() {}

void r_tele_script(uint8_t a) {
    trace("script: %i\n", a);
    tele_engine_run_script(active_engine, a - 1 /* 0-indexed */);
}

void r_tele_kill() {}

void r_tele_mute(uint8_t i, uint8_t s) {}

void r_tele_input_state(uint8_t n) {}

void redirect_tele_callbacks() {
    update_metro = &r_tele_metro;
    update_tr = &r_tele_tr;
    update_cv = &r_tele_cv;
    update_cv_slew = &r_tele_cv_slew;
    update_delay = &r_tele_delay;
    update_s = &r_tele_s;
    update_cv_off = &r_tele_cv_off;
    update_ii = &r_tele_ii;
    update_scene = &r_tele_scene;
    update_pi = &r_tele_pi;
    run_script = &r_tele_script;
    update_kill = &r_tele_kill;
    update_mute = &r_tele_mute;
    update_input = &r_tele_input_state;
}


////////////////////////////////////////////////////////////////////////////////

void tele_engine_set_scene(tele_engine_t *engine, int index) {
    // TODO: range macro.
    engine->scene = &engine->scenes[index];
}

void tele_engine_process_cmd(tele_engine_t *engine, char *cmd, char *result) {
    parse_result_t r = {};
    error_t status = parse_command(cmd, &r);
    if (status == E_OK) {
        process_result_t out = process(&r.cmd);
        if (out.has_value) { sprintf(result, "%i", out.value); }
    }
    else {
        strcpy(result, tele_error(status));
    }
}

void tele_engine_run_script(tele_engine_t *engine, int index) {
    trace("running script index: %i\n", index);
    // TODO: range macro.
    tele_script_t *script = tele_engine_get_script(engine, index);
    for (int i = 0; i < script->l; ++i) {
        trace("  %s\n", print_command(&script->c[i]));
        process(&script->c[i]);
    }
}

void tele_engine_load_scene(tele_engine_t *engine, char *scene, int number) {
    tele_scene_t *s = &engine->scenes[number];
    tele_scene_init(s);
    tele_engine_set_scene(engine, number);
    // Keep scene reading from getting tripped up.
    remove_all(scene, '\r');
    // TODO: validate?
    tele_engine_read_scene(s, scene);
}

tele_script_t *tele_engine_get_script(tele_engine_t *engine, int index) {
    return &engine->scene->script[index];
}

error_t parse_command(const char *cmd, parse_result_t *result) {
    error_t status;
    status = parse((char *)cmd, &result->cmd);
    if (status != E_OK) { strcpy(result->error_detail, error_detail); }
    else {
        status = validate(&result->cmd);
        if (status != E_OK) { strcpy(result->error_detail, error_detail); }
    }
    return status;
}

void tele_scene_init(tele_scene_t *scene) {
    // TODO: cleanup inits
    // Patterns.
    for (int j = 0; j < MAX_PATTERNS; ++j) {
        scene->patterns[j].i = 0;
        scene->patterns[j].l = 0;
        scene->patterns[j].wrap = 1;
        scene->patterns[j].start = 0;
        scene->patterns[j].end = 63;
    }
    // Scripts.
    for (int j = 0; j < MAX_SCRIPTS; ++j) {
        scene->script[j].l = 0;
        for (int k = 0; k < SCRIPT_MAX_COMMANDS; ++k) {
            scene->script[j].c[k].l = 0;
            scene->script[j].c[k].separator = -1;
            for (int m = 0; m < COMMAND_MAX_LENGTH; ++m) {
                scene->script[j].c[k].data[m].v = 0;
                scene->script[j].c[k].data[m].t = NUMBER;  //?
            }
        }
    }
    // Text.
    memset(scene->text, 0, sizeof(scene->text));
}

// Cribbed from main.c.
void tele_engine_read_scene(tele_scene_t *scene, char *text) {
    int8_t s = 99;
    char c;
    uint8_t l = 0;
    uint8_t p = 0;
    uint8_t b = 0;
    uint16_t num = 0;
    int8_t neg = 1;
    char input[32] = {};

    for (char *ch = text; *ch && s != -1; ++ch) {
        c = toupper(*ch);
        if (c == '#') {
            ++ch;
            if (*ch) {
                c = toupper(*ch);
                if (c == 'M')
                    s = 8;
                else if (c == 'I')
                    s = 9;
                else if (c == 'P')
                    s = 10;
                else {
                    s = c - 49;
                    if (s < 0 || s > 7) s = -1;
                }

                l = 0;
                p = 0;

                ++ch;
                if (*ch) { c = toupper(*ch); }
            }
            else
                s = -1;
        }
        // SCENE TEXT
        else if (s == 99) {
            if (c == '\n') {
                l++;
                p = 0;
            }
            else {
                if (l < SCENE_TEXT_LINES && p < SCENE_TEXT_CHARS) {
                    scene->text[l][p] = c;
                    p++;
                }
            }
        }
        // SCRIPTS
        else if (s >= 0 && s <= 9) {
            if (c == '\n') {
                if (p && l < SCRIPT_MAX_COMMANDS) {
                    tele_command_t temp;
                    error_t status = parse(input, &temp);

                    if (status == E_OK) {
                        status = validate(&temp);

                        if (status == E_OK) {
                            memcpy(&scene->script[s].c[l], &temp,
                                   sizeof(tele_command_t));
                            memset(input, 0, sizeof(input));
                            scene->script[s].l++;
                        }
                        else {
                            trace("%s\n", tele_error(status));
                        }
                    }
                    else {
                        trace("\r\nERROR: ");
                        trace("%s\n", tele_error(status));
                        trace(" >> ");
                        trace("%s\n", print_command(&scene->script[s].c[l]));
                    }

                    l++;
                    p = 0;
                }
            }
            else {
                if (p < 32) input[p] = c;
                p++;
            }
        }
        // PATTERNS
        else if (s == 10) {
            if (c == '\n' || c == '\t') {
                if (b < 4) {
                    if (l > 3) { scene->patterns[b].v[l - 4] = neg * num; }
                    else if (l == 0) {
                        scene->patterns[b].l = num;
                    }
                    else if (l == 1) {
                        scene->patterns[b].wrap = num;
                    }
                    else if (l == 2) {
                        scene->patterns[b].start = num;
                    }
                    else if (l == 3) {
                        scene->patterns[b].end = num;
                    }
                }

                b++;
                num = 0;
                neg = 1;

                if (c == '\n') {
                    if (p) l++;
                    if (l > 68) s = -1;
                    b = 0;
                    p = 0;
                }
            }
            else {
                if (c == '-')
                    neg = -1;
                else if (c >= '0' && c <= '9') {
                    num = num * 10 + (c - 48);
                }
                p++;
            }
        }
    }
}

void remove_all(char *str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}
