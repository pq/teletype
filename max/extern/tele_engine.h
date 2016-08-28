#ifndef _TELE_ENGINE_H_
#define _TELE_ENGINE_H_

#include <stdbool.h>

#include "state.h"
#include "teletype.h"


#define TRACE 1

#define trace(...)                               \
    do {                                         \
        if (TRACE) fprintf(stderr, __VA_ARGS__); \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// from main (consider moving to teletype.h)

#define MAX_PATTERNS 4
#define MAX_SCENES 8
#define MAX_SCRIPTS 10

#define METRO_SCRIPT 8
#define INIT_SCRIPT 9

#define SCENE_TEXT_LINES 32
#define SCENE_TEXT_CHARS 32

// Removed const modifier.
typedef /* const */ struct {
    tele_script_t script[MAX_SCRIPTS];
    tele_pattern_t patterns[MAX_PATTERNS];
    char text[SCENE_TEXT_LINES][SCENE_TEXT_CHARS];
} tele_scene_t;

// end from main
////////////////////////////////////////////////////////////////////////////////


// Describes what engine values are being updated (and how) to identify what
// corresponding ports need refresh.
typedef struct {
    // 1-indexed
    uint8_t cv;  // :3
    uint8_t tr;  // :3
    bool metro;
    uint8_t metro_act;  // :1
    bool slew;
    bool v;
} tele_engine_update_t;

typedef void (*tele_engine_update_cb_t)(void *engine,
                                        tele_engine_update_t event, void *data);

typedef struct {
    tele_engine_update_cb_t callback;
    void *data;
} tele_engine_watcher;

typedef struct {
    tele_scene_t scenes[MAX_SCENES];
    tele_engine_watcher *watcher;
    tele_scene_t *scene;
    scene_state_t *state;
} tele_engine_t;


////////////////////////////////////////////////////////////////////////////////
//
// Initialization and life-cycle.
//
////////////////////////////////////////////////////////////////////////////////

// Consider breaking out a separate tele_engine_watch(...) function?
tele_engine_t *tele_engine_new(tele_engine_watcher watcher);

void tele_engine_free(tele_engine_t *engine);


////////////////////////////////////////////////////////////////////////////////
//
// Processing.
//
////////////////////////////////////////////////////////////////////////////////

void tele_engine_set_scene(tele_engine_t *engine, int index);

void tele_engine_load_scene(tele_engine_t *engine, char *scene, int number);

void tele_engine_run_script(tele_engine_t *engine, int index);

void tele_engine_process_cmd(tele_engine_t *engine, char *cmd, char *result);


#endif  //_TELE_ENGINE_H_
