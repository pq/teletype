#include <string.h>

#include "greatest.h"

#include "tele_engine.h"
#include "tele_engine_tests.h"


////////////////////////////////////////////////////////////////////////////////
//
// Test Helpers
//
////////////////////////////////////////////////////////////////////////////////

// Shared test engine.
tele_engine_t *engine;

void engine_updated(void *engine, tele_engine_update_t context, void *data) {
    trace("engine updated\n");
}

static void setup_engine(void *_) {
    engine =
        tele_engine_new((tele_engine_watcher){.callback = &engine_updated });
}
static void teardown_engine(void *_) {
    tele_engine_free(engine);
}


////////////////////////////////////////////////////////////////////////////////
//
// Tests
//
////////////////////////////////////////////////////////////////////////////////


TEST test_read_scene() {
    tele_scene_t scene;
    tele_scene_init(&scene);
    tele_engine_read_scene(&scene, tt01_txt);

    //  8	8	8	8
    //  1	1	1	1
    //  0	0	0	0
    //  5	7	7	7
    ASSERT_EQ(8, scene.patterns[0].l);
    ASSERT_EQ(1, scene.patterns[0].wrap);
    ASSERT_EQ(0, scene.patterns[0].start);
    ASSERT_EQ(5, scene.patterns[0].end);
    ASSERT_EQ(7, scene.patterns[1].end);

    //  0	0	0	0
    //  4	2	3	5
    //  7	5	6	7
    //  11	9	8	10
    //  12	12	11	14
    //  16	15	15	19
    //  19	19	17	21
    //  23	22	22	26
    ASSERT_EQ(0, scene.patterns[0].v[0]);
    ASSERT_EQ(4, scene.patterns[0].v[1]);
    ASSERT_EQ(7, scene.patterns[0].v[2]);
    ASSERT_EQ(11, scene.patterns[0].v[3]);
    ASSERT_EQ(12, scene.patterns[0].v[4]);
    ASSERT_EQ(16, scene.patterns[0].v[5]);
    ASSERT_EQ(19, scene.patterns[0].v[6]);
    ASSERT_EQ(23, scene.patterns[0].v[7]);

    PASS();
}

TEST test_load_scene() {
    char scene[] =
        "#I\n"
        "X 9\n"
        "#1\n"
        "CV 1 X\n"
        "CV 2 2\n"
        "CV 3 3\n"
        "CV 4 4\n";

    tele_engine_load_scene(engine, scene, 1);

    tele_script_t *init = tele_engine_get_script(engine, INIT_SCRIPT);
    ASSERT_EQ(1, init->l);
    ASSERT_COMMAND_EQ("X 9", &init->c[0]);

    tele_script_t *one = tele_engine_get_script(engine, 0);
    ASSERT_EQ(4, one->l);
    ASSERT_COMMAND_EQ("CV 1 X", &one->c[0]);

    // Unwritten scripts should be defined but empty.
    for (int i = 1; i < 8; ++i) {
        ASSERT_EQ(0, tele_engine_get_script(engine, i)->l);
    }

    PASS();
}

TEST test_initial_state() {
    ASSERT_EQ(&engine->scenes[DEFAULT_SCENE], engine->scene);
    ASSERT_EQ(&engine->scenes[DEFAULT_SCENE].script[0],
              tele_engine_get_script(engine, 0));
    PASS();
}

SUITE(engine_tests) {
    GREATEST_SET_SETUP_CB(setup_engine, NULL);
    GREATEST_SET_TEARDOWN_CB(teardown_engine, NULL);
    RUN_TEST(test_load_scene);
    RUN_TEST(test_read_scene);
    RUN_TEST(test_initial_state);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(engine_tests);
    GREATEST_MAIN_END();
}
