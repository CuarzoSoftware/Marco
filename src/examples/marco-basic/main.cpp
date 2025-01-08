#include <Marco/MApplication.h>
#include <Marco/MScreen.h>
#include <Marco/roles/MToplevel.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSimpleText.h>
#include <AK/nodes/AKButton.h>
#include <AK/AKTheme.h>
#include <iostream>

using namespace Marco;
using namespace AK;

int main()
{
    MApplication app;
    app.setAppId("org.Cuarzo.marco-basic");

    app.on.screenPlugged.subscribe(&app, [](MScreen &screen){
        std::cout << "New Screen! " << screen.props().name << std::endl;
    });

    app.on.screenUnplugged.subscribe(&app, [](MScreen &screen){
        std::cout << "Bye bye Screen! " << screen.props().name << std::endl;
    });

    MToplevel window;
    window.setTitle("Hello world!");
    window.layout().setAlignItems(YGAlignCenter);
    window.layout().setJustifyContent(YGJustifyCenter);
    window.layout().setPadding(YGEdgeAll, 48.f);
    window.layout().setGap(YGGutterAll, 8.f);

    AKSimpleText helloWorld { "Hello World!", &window };
    AKButton maximizeButton { "Toggle Maximized", &window };
    AKButton fullscreenButton { "Toggle Fullscreen", &window };
    AKButton minimizeButton { "Minimize", &window };
    AKButton exitButton { "Exit", &window };

    maximizeButton.on.clicked.subscribe(&maximizeButton, [&window](){
        window.setMaximized(!window.states().check(MToplevel::Maximized));
    });

    fullscreenButton.on.clicked.subscribe(&fullscreenButton, [&window](){
        window.setFullscreen(!window.states().check(MToplevel::Fullscreen));
    });

    minimizeButton.on.clicked.subscribe(&minimizeButton, [&window](){
        window.setMinimized();
    });

    exitButton.on.clicked.subscribe(&exitButton, [](){
        exit(0);
    });

    return app.exec();
}
