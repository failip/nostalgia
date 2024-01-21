#include "engine/engine.h"

int main(int, char**) {
    Engine engine = Engine(640, 480);
    if (!engine.on_init()) return 1;
    
    while (engine.is_running()) {
        engine.on_frame();
    }

    engine.on_finish();
    return 0;
}