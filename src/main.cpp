// =============================================================================
//  main.cpp - Application entry point (wWinMain).
//  Creates the MainWindow instance and runs the message loop.
// =============================================================================

#include "ui/MainWindow.h"
#include "resource.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow)
{
    // Prevent unused-parameter warnings.
    (void)hPrevInstance;
    (void)lpCmdLine;

    MainWindow app;
    if (!app.create(hInstance, nCmdShow))
        return 1;

    return app.runMessageLoop();
}
