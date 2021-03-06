#ifndef _TELETYPE_H_
#define _TELETYPE_H_

#include <stdbool.h>
#include <stdint.h>

#include "state.h"

#define SCRIPT_MAX_COMMANDS 6
#define SCRIPT_MAX_COMMANDS_ 5
#define COMMAND_MAX_LENGTH 12
#define STACK_SIZE 8
#define TELE_STACK_SIZE 8
#define TELE_D_SIZE 8

#define WELCOME "TELETYPE 1.12"


typedef enum {
    E_OK,
    E_WELCOME,
    E_PARSE,
    E_LENGTH,
    E_NEED_PARAMS,
    E_EXTRA_PARAMS,
    E_NO_MOD_HERE,
    E_MANY_SEP,
    E_NEED_SEP,
    E_PLACE_SEP,
    E_NOT_LEFT
} error_t;

typedef struct {
    bool has_value;
    int16_t value;
} process_result_t;

typedef enum { NUMBER, MOD, SEP, OP } tele_word_t;

typedef struct {
    tele_word_t t;
    int16_t v;
} tele_data_t;

typedef struct {
    uint8_t l;
    signed char separator;
    tele_data_t data[COMMAND_MAX_LENGTH];
} tele_command_t;

typedef struct {
    uint8_t l;
    tele_command_t c[SCRIPT_MAX_COMMANDS];
} tele_script_t;

typedef struct {
    const char *name;
    void (*func)(tele_command_t *c);
    char params;
    const char *doc;
} tele_mod_t;

typedef struct {
    const char *name;
    void (*get)(const void *data, scene_state_t *ss, exec_state_t *es,
                command_state_t *cs);
    void (*set)(const void *data, scene_state_t *ss, exec_state_t *es,
                command_state_t *cs);
    uint8_t params;
    bool returns;
    const void *data;
    const char *doc;
} tele_op_t;

typedef struct {
    int16_t i;
    uint16_t l;
    uint16_t wrap;
    int16_t start;
    int16_t end;
    int16_t v[64];
} tele_pattern_t;


error_t parse(char *cmd, tele_command_t *out);
error_t validate(tele_command_t *c);
process_result_t process(tele_command_t *c);
char *print_command(const tele_command_t *c);

void tele_tick(uint8_t);

void clear_delays(void);

void tele_init(void);

void tele_set_in(int16_t value);
void tele_set_param(int16_t value);
void tele_set_scene(int16_t value);

const char *tele_error(error_t);
const char *to_v(int16_t);

extern tele_pattern_t tele_patterns[4];

typedef void (*update_metro_t)(int16_t, int16_t, uint8_t);
extern volatile update_metro_t update_metro;

typedef void (*update_tr_t)(uint8_t, int16_t);
extern volatile update_tr_t update_tr;

typedef void (*update_cv_t)(uint8_t, int16_t, uint8_t);
extern volatile update_cv_t update_cv;

typedef void (*update_cv_slew_t)(uint8_t, int16_t);
extern volatile update_cv_slew_t update_cv_slew;

typedef void (*update_delay_t)(uint8_t);
extern volatile update_delay_t update_delay;

typedef void (*update_s_t)(uint8_t);
extern volatile update_s_t update_s;

typedef void (*update_cv_off_t)(uint8_t, int16_t v);
extern volatile update_cv_off_t update_cv_off;

typedef void (*update_ii_t)(uint8_t, int16_t);
extern volatile update_ii_t update_ii;

typedef void (*update_scene_t)(uint8_t);
extern volatile update_scene_t update_scene;

typedef void (*update_pi_t)(void);
extern volatile update_pi_t update_pi;

typedef void (*run_script_t)(uint8_t);
extern volatile run_script_t run_script;

typedef void (*update_kill_t)(void);
extern volatile update_kill_t update_kill;

typedef void (*update_mute_t)(uint8_t, uint8_t);
extern volatile update_mute_t update_mute;

typedef void (*update_input_t)(uint8_t);
extern volatile update_input_t update_input;

extern char error_detail[16];

extern volatile uint8_t input_states[8];

#endif
