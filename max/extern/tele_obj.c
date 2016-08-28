#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ext.h"
#include "ext_obex.h"

#include "tele_engine.h"
#include "tele_obj.h"
#include "teletype.h"

#define CV_OUTS 4
#define TR_OUTS 4

#define TR_INS 8

#define METRO_IN 8
#define CONSOLE_IN 9

#define INLETS (TR_INS + 2)  // TR 1-8, METRO, CONSOLE
#define PROXIES (INLETS - 1)

#define TICK_INTERVAL 10

#define DEFAULT_SCENE 0

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


typedef struct _tele {
    t_object ob;
    long m_in;
    void *m_proxy[INLETS - 1];
    void *cv_out[CV_OUTS];
    void *tr_out[TR_OUTS];
    void *metro_out;
    void *metro_act_out;
    void *console_out;
    void *debug_out;
    tele_engine_t *engine;
    t_object *t_editor;
    void *clock;
    char **t_text;
    long t_size;
} t_tele;

void tele_bang(t_tele *t);
void tele_anything(t_tele *t, t_symbol *s, long argc, t_atom *argv);
void *tele_new(t_symbol *s, long argc, t_atom *argv);
void tele_free(t_tele *x);
void tele_assist(t_tele *x, void *b, long m, long a, char *s);
void tele_dblclick(t_tele *x);
void tele_edclose(t_tele *t, char **text, long size);

void *tele_class;

void clock_tick(t_tele *t);

void load_scene(t_tele *t);

void ext_main(void *r) {
    t_class *c;

    c = class_new("teletype", (method)tele_new, (method)tele_free,
                  (long)sizeof(t_tele), 0L, A_GIMME, 0);

    class_addmethod(c, (method)tele_bang, "bang", 0);
    class_addmethod(c, (method)tele_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)tele_dblclick, "dblclick", A_CANT, 0);
    class_addmethod(c, (method)tele_edclose, "edclose", A_CANT, 0);
    class_addmethod(c, (method)tele_assist, "assist", A_CANT, 0);

    class_register(CLASS_BOX, c);
    tele_class = c;
}

void tele_assist(t_tele *t, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET) {
        if (a >= 0 && a < TR_INS) { sprintf(s, "Script trigger: %ld", a + 1); }
        if (a == METRO_IN) { sprintf(s, "Metro trigger"); }
        if (a == CONSOLE_IN) { sprintf(s, "Console in"); }
    }
    else {
        // TODO: add outlets
    }
}

void update_outlets(tele_engine_t *engine, tele_engine_update_t context,
                    t_tele *t) {
    trace("updating outlets...\n");
    if (context.cv) {
        int index = context.cv - 1;
        // TODO: this is a little revealing...
        outlet_int(t->cv_out[index], engine->state->variables.cv[index]);
        trace("  out[%i] = %i\n", index, engine->state->variables.cv[index]);
    }
    if (context.tr) {
        int index = context.tr - 1;
        // TODO: move value into context object
        outlet_int(t->tr_out[index], engine->state->variables.tr[index]);
        trace("  tr[%i] => %i\n", index, engine->state->variables.tr[index]);
    }
    if (context.metro) {
        outlet_int(t->metro_act_out, context.metro_act);
        outlet_int(t->metro_out, engine->state->variables.m);
    }

    //    char msg[80];
    //    sprintf(msg, "x = %i", engine->state->variables.x);
    //    outlet_anything(t->debug_out, gensym(msg), 0, NULL);
}

void tele_bang(t_tele *t) {
    int in = proxy_getinlet((t_object *)t);
    trace("bang received at: %i\n", in);
    tele_engine_run_script(t->engine, in);
}


void tele_anything(t_tele *t, t_symbol *s, long argc, t_atom *argv) {
    int in = proxy_getinlet((t_object *)t);
    // TODO: cleanup and validate
    // TODO: send empty string to clear console (when no output)
    if (in == CONSOLE_IN) {
        char cmd[64] = {};
        for (int i = 0; i < argc; ++i) {
            trace("atom type: %ld\n", atom_gettype(&argv[i]));
            if (atom_gettype(&argv[i]) == A_SYM) {
                t_symbol *my_sym = atom_getsym(&argv[i]);
                trace("  token: %s\n", my_sym->s_name);
                strcat(cmd, my_sym->s_name);
                strcat(cmd, " ");
            }
            if (atom_gettype(&argv[i]) == A_LONG) {
                long my_long = atom_getlong(&argv[i]);
                trace("  token: %ld\n", my_long);
                char c[16] = {};
                sprintf(c, "%ld ", my_long);
                strcat(cmd, c);
            }
        }
        trace("read: %s\n", cmd);
        char result[64] = {};
        tele_engine_process_cmd(t->engine, cmd, result);
        trace("got result: %s\n", result);
        outlet_anything(t->console_out, gensym(result), 0, NULL);
    }
}

void tele_dblclick(t_tele *t) {
    if (t->t_editor)
        object_attr_setchar(t->t_editor, gensym("visible"), 1);
    else {
        t->t_editor = object_new(CLASS_NOBOX, gensym("jed"), t, 0);
        object_method(t->t_editor, gensym("settext"), *t->t_text,
                      gensym("utf-8"));
        object_attr_setchar(t->t_editor, gensym("scratch"), 1);
        object_attr_setsym(t->t_editor, gensym("title"), gensym("teletype"));
    }
}

void tele_edclose(t_tele *t, char **text, long size) {
    if (t->t_text) sysmem_freehandle(t->t_text);

    t->t_text = sysmem_newhandleclear(size + 1);
    sysmem_copyptr((char *)*text, *t->t_text, size);
    t->t_size = size + 1;
    t->t_editor = NULL;
    load_scene(t);
}

void load_scene(t_tele *t) {
    trace("loading scene ...\n");
    tele_engine_load_scene(t->engine, *t->t_text, DEFAULT_SCENE);
    tele_engine_run_script(t->engine, INIT_SCRIPT);
}

void engine_updated(void *engine, tele_engine_update_t context, void *data) {
    trace("engine updated\n");
    update_outlets(engine, context, (t_tele *)data);
}

void *tele_new(t_symbol *s, long argc, t_atom *argv) {
    t_tele *t = NULL;

    t = (t_tele *)object_alloc(tele_class);

    long size = sizeof(STARTER_SCRIPT);

    t->t_text = sysmem_newhandleclear(size + 1);
    sysmem_copyptr((char *)STARTER_SCRIPT, *t->t_text, size);

    t->t_size = sysmem_handlesize(t->t_text);
    t->t_editor = NULL;


    t->clock = clock_new((t_object *)t, (method)clock_tick);

    t->engine = tele_engine_new(
        (tele_engine_watcher){.data = t, .callback = &engine_updated });

    // Inlets (less one for the default leftmost).
    for (int i = PROXIES; i > 0; --i) {
        t->m_proxy[i - 1] = proxy_new((t_object *)t, i, &t->m_in);
    }

    // Outlets (right to left).

    t->console_out = outlet_new((t_object *)t, 0L);  //?
    t->debug_out = outlet_new((t_object *)t, 0L);    //?

    // METRO
    t->metro_act_out = intout((t_object *)t);
    t->metro_out = intout((t_object *)t);

    // TR
    for (int i = TR_OUTS - 1; i >= 0; --i) {
        t->tr_out[i] = intout((t_object *)t);
    }

    // CV
    for (int i = CV_OUTS - 1; i >= 0; --i) {
        t->cv_out[i] = intout((t_object *)t);
    }

    // Start clock.
    clock_delay(t->clock, 0L);

    return t;
}

void clock_tick(t_tele *t) {
    clock_delay(t->clock, TICK_INTERVAL);
    tele_tick(TICK_INTERVAL);
}

void tele_free(t_tele *t) {
    if (t->t_text) sysmem_freehandle(t->t_text);
    object_free(t->clock);
    for (int i = 0; i < PROXIES; ++i) { object_free(t->m_proxy[i]); }

    if (t->engine) tele_engine_free(t->engine);
}
