#pragma once

namespace huEVO {

class Application {
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

    void RequestExit() { m_Running = false; }

private:
    bool m_Running;
};

extern Application g_App;

} // namespace huEVO
