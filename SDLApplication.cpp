#include "SDLApplication.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <string>

// SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
// SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);

class SDLApplicationImpl
{
    SDL_Window *m_pCompanionWindow = NULL;
    uint32_t m_nCompanionWindowWidth = 640;
    uint32_t m_nCompanionWindowHeight = 320;

public:
    ~SDLApplicationImpl()
    {
        SDL_StopTextInput();

        if (m_pCompanionWindow)
        {
            SDL_DestroyWindow(m_pCompanionWindow);
            m_pCompanionWindow = NULL;
        }

        SDL_Quit();
    }

    int Width() const { return m_nCompanionWindowWidth; }
    int Height() const { return m_nCompanionWindowHeight; }

    bool Initialize(const std::string &driver, const std::string &display)
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
        {
            // dprintf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
            return false;
        }

        int nWindowPosX = 700;
        int nWindowPosY = 100;
        Uint32 unWindowFlags = SDL_WINDOW_SHOWN;

        m_pCompanionWindow = SDL_CreateWindow("hellovr [D3D12]", nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);
        if (m_pCompanionWindow == NULL)
        {
            // dprintf("%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
            return false;
        }

        std::string strWindowTitle = "hellovr [D3D12] - " + driver + " " + display;
        SDL_SetWindowTitle(m_pCompanionWindow, strWindowTitle.c_str());

        SDL_StartTextInput();
        SDL_ShowCursor(SDL_DISABLE);

        return true;
    }

    HWND HWND()
    {
        // Determine the HWND from SDL
        struct SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(m_pCompanionWindow, &wmInfo);
        return wmInfo.info.win.window;
    }

    bool HandleInput(bool *bShowCubes)
    {
        SDL_Event sdlEvent;
        bool bRet = false;
        while (SDL_PollEvent(&sdlEvent) != 0)
        {
            if (sdlEvent.type == SDL_QUIT)
            {
                bRet = true;
            }
            else if (sdlEvent.type == SDL_KEYDOWN)
            {
                if (sdlEvent.key.keysym.sym == SDLK_ESCAPE || sdlEvent.key.keysym.sym == SDLK_q)
                {
                    bRet = true;
                }
                if (sdlEvent.key.keysym.sym == SDLK_c)
                {
                    *bShowCubes = !(*bShowCubes);
                }
            }
        }

        return bRet;
    }
};

SDLApplication::SDLApplication()
    : m_impl(new SDLApplicationImpl)
{
}
SDLApplication::~SDLApplication()
{
    delete m_impl;
}
bool SDLApplication::Initialize(const std::string &driver, const std::string &display)
{
    return m_impl->Initialize(driver, display);
}
int SDLApplication::Width() const
{
    return m_impl->Width();
}
int SDLApplication::Height() const
{
    return m_impl->Height();
}
void *SDLApplication::HWND() const
{
    return m_impl->HWND();
}
bool SDLApplication::HandleInput(bool *bShowCubes)
{
    return m_impl->HandleInput(bShowCubes);
}
