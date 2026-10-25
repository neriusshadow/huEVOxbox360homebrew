#include "app.h"

#ifdef _XBOX
#include <xtl.h>

void __cdecl main() {
    if (huEVO::g_App.Initialize()) {
        huEVO::g_App.Run();
    }
    huEVO::g_App.Shutdown();
}

#else

int main(int argc, char* argv[]) {
    if (huEVO::g_App.Initialize()) {
        huEVO::g_App.Run();
    }
    huEVO::g_App.Shutdown();
    return 0;
}

#endif
