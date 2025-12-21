#include "Cleanpad.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    Cleanpad app;
    return app.Run(hInstance, nCmdShow);
}