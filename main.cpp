#include "store.h"

// options:  --plain  = no colors     --color = force colors (for example when piping into "less -R")
int main(int argc, char* argv[]) {
    bool allowColor = true;
    bool forceColor = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--plain") allowColor = false;
        if (arg == "--color") forceColor = true;
    }
    tuiInit(allowColor, forceColor);

    runStore();
    return 0;
}
