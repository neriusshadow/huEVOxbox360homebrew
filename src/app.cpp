#include "app.h"
#include "renderer.h"
#include "input.h"
#include "network.h"
#include "ui.h"
#include "config.h"

#ifdef _XBOX
#include <xtl.h>
#endif

namespace huEVO {

Application g_App;
AppConfig g_Config;

Application::Application()
    : m_Running(false)
{
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    // 1. Initialize Direct3D 9
    if (!g_Renderer.Initialize()) {
        return false;
    }

    // 2. Initialize UI
    g_UI.Initialize();

    // 3. Render first frame immediately to display TV image without any freeze
    g_Renderer.BeginFrame();
    g_UI.Render();
    g_Renderer.EndFrame();

    // 4. Initialize Networking (XNet / Winsock)
    g_Network.Initialize();

    m_Running = true;
    return true;
}

void Application::Shutdown() {
    g_Network.Shutdown();
    g_Renderer.Shutdown();
}

void Application::Run() {
    while (m_Running) {
        // 1. Poll Gamepad Input
        g_Input.Update();

        // 2. Check for Exit command (Back button returns cleanly to Aurora / Dashboard)
        if (g_Input.IsPressed(XINPUT_GAMEPAD_BACK)) {
            m_Running = false;
            break;
        }

        // 3. Update App Logic and UI
        g_UI.Update();

        // 4. Render Frame
        g_Renderer.BeginFrame();
        g_UI.Render();
        g_Renderer.EndFrame();
    }

#ifdef _XBOX
    // Graceful exit back to Aurora Dashboard or default launcher
    XLaunchNewImage(NULL, 0);
#endif
}

} // namespace huEVO
