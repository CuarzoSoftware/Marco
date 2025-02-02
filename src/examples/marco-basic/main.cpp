#include <Marco/MApplication.h>
#include <Marco/MScreen.h>
#include <Marco/roles/MToplevel.h>
#include <Marco/utils/MImageLoader.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSimpleText.h>
#include <AK/nodes/AKButton.h>
#include <AK/nodes/AKSolidColor.h>
#include <AK/nodes/AKImageFrame.h>
#include <AK/nodes/AKTextField.h>
#include <AK/effects/AKBackgroundBoxShadowEffect.h>
#include <AK/AKTheme.h>
#include <iostream>

using namespace Marco;
using namespace AK;

class Window : public MToplevel
{
public:
    Window() noexcept : MToplevel() {
        setColorWithAlpha(0xffF0F0F0);
        setTitle("Hello world!");
        topbar.enableDiminishOpacityOnInactive(true);
        topbar.layout().setAlignItems(YGAlignCenter);
        topbar.layout().setJustifyContent(YGJustifyCenter);
        SkFont font;
        font.setTypeface(SkTypeface::MakeFromName("Inter",
                                                  SkFontStyle(SkFontStyle::kExtraBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant)));
        font.setSize(14);
        helloWorld.setColorWithAlpha(0xb3000000);
        helloWorld.setFont(font);
        helloWorld.enableDiminishOpacityOnInactive(true);
        topbar.layout().setHeight(32);
        topbar.layout().setWidthPercent(100);
        body.layout().setFlex(1.f);
        body.layout().setAlignItems(YGAlignCenter);
        body.layout().setJustifyContent(YGJustifyCenter);
        body.layout().setPadding(YGEdgeAll, 48.f);
        body.layout().setGap(YGGutterAll, 8.f);
        cat.layout().setWidth(200);
        cat.layout().setHeight(200);
        newWindowButton.setBackgroundColor(AKTheme::SystemBlue);
        exitButton.setBackgroundColor(AKTheme::SystemRed);

        disabledButton.setEnabled(false);

        cursorButton.on.clicked.subscribe(this, [this](){
            if (cursor == 34)
                cursor = 0;
            else
                cursor++;

            cursorButton.setText(std::string("Cursor: ") + cursorToString((AKCursor)cursor));
            pointer().setCursor((AKCursor)cursor);
        });

        newWindowButton.on.clicked.subscribe(this, [](){
            new Window();
        });

        maximizeButton.on.clicked.subscribe(this, [this](){
           setMaximized(!states().check(MToplevel::Maximized));
        });

        fullscreenButton.on.clicked.subscribe(this, [this](){
            setFullscreen(!states().check(MToplevel::Fullscreen));
        });

        minimizeButton.on.clicked.subscribe(this, [this](){
            setMinimized();
        });

        exitButton.on.clicked.subscribe(&exitButton, [](){
            exit(0);
        });

        //cat.setAnimated(true);
    }

    AKSolidColor topbar { 0xFFFAFAFA, this};
    AKSimpleText helloWorld { "Hello World!", &topbar };
    AKBackgroundBoxShadowEffect shadow {2, {0,0}, 0x80000000, false, &topbar};
    AKContainer body { YGFlexDirectionColumn, true, this };
    AKImageFrame cat { MImageLoader::loadFile("/home/eduardo/cat.jpg"), &body };
    UInt32 cursor { 1 };
    AKButton cursorButton { "Cursor: Default", &body };
    AKButton newWindowButton { "New Window", &body };
    AKButton maximizeButton { "Toggle Maximized", &body };
    AKButton fullscreenButton { "Toggle Fullscreen", &body };
    AKButton minimizeButton { "Minimize", &body };
    AKButton disabledButton { "Disabled Button", &body };
    AKButton exitButton { "Exit", &body };
    AKTextField textField { &body };
};

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

    Window window;
    window.::MSurface::on.presented.subscribe(&window, [&window](UInt32 ms){
        //window.cat.renderableImage().setOpacity(1.f + 0.5f*SkScalarCos(ms * 0.005f));
        std::cout << "Presented" << ms << std::endl;
    });
    return app.exec();
}
