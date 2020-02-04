#pragma
#include <string>

//-----------------------------------------------------------------------------
// Purpose: Window management
//-----------------------------------------------------------------------------
class SDLApplication
{
    class SDLApplicationImpl *m_impl = nullptr;

public:
    SDLApplication();
    ~SDLApplication();
    bool Initialize(const std::string& driver, const std::string &display);
    int Width() const;
    int Height() const;
    void *HWND() const;
    bool HandleInput(bool *bShowCubes);
};
