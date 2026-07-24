#include <CZ/Marco/MApp.h>
#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MShadowDecorations.h>
#include <CZ/AK/Nodes/AKButton.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/AK/AKLog.h>
#include <CZ/Core/Events/CZPointerButtonEvent.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace CZ;

static const std::vector<std::string> anchorGravityNames {
    "None",
    "Top",
    "Bottom",
    "Left",
    "Right",
    "TopLeft",
    "BottomLeft",
    "TopRight",
    "BottomRight"
};

class Menu: public MPopup
{
public:
    Menu(MSurface *parent) noexcept : MPopup()
    {
        setColor(SkColorSetARGB(255, rand() % 255, rand() % 255, rand() % 255));
        setParent(parent);
        setAnchor(parent->role() == Role::Toplevel ? Anchor::TopLeft : Anchor::BottomRight);
        layout().setGap(YGGutterAll, 16.f);
        layout().setPadding(YGEdgeAll, 16.f);
        const SkISize contentSize { minContentSize() };
        layout().setWidth(contentSize.width());
        layout().setHeight(contentSize.height());

        closeButton.onClick.subscribe(this, [this](const auto &){ setMapped(false); });
        showButton.onClick.subscribe(this, [this](const auto &){
            if (!subMenu)
                subMenu = std::make_unique<Menu>(this);
            subMenu->setMapped(true);
        });

        anchorButton.onClick.subscribe(this, [this](const auto &){
            UInt32 a = (UInt32)anchor();
            if (a == 8) a = 0;
            else a++;
            setAnchor((Anchor)a);
            anchorButton.textNode().setText("Anchor: " + anchorGravityNames[a]);
        });

        gravityButton.onClick.subscribe(this, [this](const auto &){
            UInt32 g = (UInt32)gravity();
            if (g == 8) g = 0;
            else g++;
            setGravity((Gravity)g);
            gravityButton.textNode().setText("Gravity: " + anchorGravityNames[g]);
        });
    }

    ~Menu() { AKLog(CZDebug, CZLN, "Popup destroyed"); }

    AKButton showButton { "Add sub popup", this };
    AKButton anchorButton { "Change anchor: TopLeft", this };
    AKButton gravityButton { "Change gravity: BottomRight", this };
    AKButton closeButton { "Close", this };
    std::unique_ptr<Menu> subMenu;
};

class Window : public MToplevel
{
public:
    Window() noexcept : MToplevel()
    {
        setTitle("Popup Example");
        layout().setGap(YGGutterAll, 16.f);
        layout().setPadding(YGEdgeAll, 16.f);
        layout().setJustifyContent(YGJustifySpaceEvenly);
        setMinSize(SkISize(400, 400));

        showButton.onClick.subscribe(this, [this](const auto &e){
            menu.setGrab(&e);
            menu.setMapped(true);
        });

        multiButton.onClick.subscribe(this, [this](const auto &e){
            Menu *menu = new Menu(this);
            menu->onMappedChanged.subscribe(menu, [menu](){
                if (!menu->mapped())
                    menu->destroyLater();
            });
            menu->setMapped(true);
            menu->showButton.onClick.notify(e);
            menu->subMenu->showButton.onClick.notify(e);
            menu->subMenu->subMenu->showButton.onClick.notify(e);
        });

        longButton.onClick.subscribe(this, [this](const auto &){
            Menu *menu = new Menu(this);
            menu->onMappedChanged.subscribe(menu, [menu](){
                if (!menu->mapped())
                    menu->destroyLater();
            });
            menu->layout().setHeight(4000);
            menu->setMapped(true);
        });

        decoratedButton.onClick.subscribe(this, [this](const auto &){
            Menu *menu = new Menu(this);
            menu->onMappedChanged.subscribe(menu, [menu](){
                if (!menu->mapped())
                    menu->destroyLater();
            });
            menu->setDecorations(std::make_unique<MShadowDecorations>());
            menu->setMapped(true);
        });

        exitButton.onClick.subscribe(this, [](const auto &){ exit(0); });
    }

    AKButton showButton { "Show popup", this};
    AKButton multiButton { "Show 4 popups at once", this};
    AKButton longButton { "Show long popup", this};
    AKButton decoratedButton { "Show decorated popup", this};
    AKButton exitButton { "Exit", this };
    Menu menu { this };
};

int main()
{
    auto app { MApp::Make() };
    app->setAppId("org.Cuarzo.marco-popup");

    Window window;
    window.setMapped(true);
    return app->run();
}
