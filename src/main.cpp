// =============================================================================
//  main.cpp - Application entry point (wWinMain).
//  Creates the MainWindow instance and runs the message loop.
// =============================================================================

#include "common.h"
#include "ui/MainWindow.h"
#include "ui/DpiHelper.h"
#include "resource.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine, int nCmdShow)
{
    // Prevent unused-parameter warnings.
    (void)hPrevInstance;
    (void)lpCmdLine;

    // Enable DPI awareness before creating any windows.
    // This is a runtime fallback for the manifest (src/app.manifest)
    // which declares PerMonitorV2 DPI awareness.
    DpiHelper::enableDpiAwareness();

    MainWindow app;
    if (!app.create(hInstance, nCmdShow))
        return 1;

    return app.runMessageLoop();
}
