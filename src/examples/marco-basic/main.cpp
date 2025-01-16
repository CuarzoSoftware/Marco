#include <Marco/MApplication.h>
#include <Marco/MScreen.h>
#include <Marco/roles/MToplevel.h>
#include <Marco/utils/MImageLoader.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSimpleText.h>
#include <AK/nodes/AKButton.h>
#include <AK/nodes/AKSolidColor.h>
#include <AK/nodes/AKImage.h>
#include <AK/effects/AKBackgroundBoxShadowEffect.h>
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
    window.setColorWithAlpha(0xffF0F0F0);
    window.setTitle("Hello world!");

    AKSolidColor topbar { 0xFFFAFAFA, &window};
    topbar.layout().setAlignItems(YGAlignCenter);
    topbar.layout().setJustifyContent(YGJustifyCenter);
    SkFont font;
    font.setTypeface(SkTypeface::MakeFromName("Inter",
                    SkFontStyle(SkFontStyle::kExtraBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant)));
    font.setSize(14);
    AKSimpleText helloWorld { "Hello World!", &topbar };
    helloWorld.setColorWithAlpha(0xb3000000);
    helloWorld.setFont(font);

    AKBackgroundBoxShadowEffect shadow {2, {0,0}, 0x80000000, false, &topbar};
    topbar.layout().setHeight(32);
    topbar.layout().setWidthPercent(100);

    AKContainer body { YGFlexDirectionColumn, true, &window };
    body.layout().setFlex(1.f);
    body.layout().setAlignItems(YGAlignCenter);
    body.layout().setJustifyContent(YGJustifyCenter);
    body.layout().setPadding(YGEdgeAll, 48.f);
    body.layout().setGap(YGGutterAll, 8.f);

    AKImage cat { MImageLoader::loadFile("/home/eduardo/cat.jpg"), &body };
    cat.layout().setWidth(200);
    cat.layout().setHeight(200);
    AKButton blueButton { "Blue button", &body };
    blueButton.setBackgroundColor(AKTheme::SystemBlue);
    AKButton maximizeButton { "Toggle Maximized", &body };
    AKButton fullscreenButton { "Toggle Fullscreen", &body };
    AKButton minimizeButton { "Minimize", &body };
    AKButton exitButton { "Exit", &body };
    exitButton.setBackgroundColor(AKTheme::SystemRed);

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
