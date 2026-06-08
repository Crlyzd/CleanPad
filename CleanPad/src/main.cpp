#include "App.h"

// ============================================================
//  main.cpp — Entry point
//  Thin as possible: just instantiate App and hand off.
// ============================================================

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_     LPWSTR    /*lpCmdLine*/,
    _In_     int       nCmdShow)
{
    App app;
    return app.Run(hInstance, nCmdShow);
}
