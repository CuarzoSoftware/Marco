#include <Marco/MApplication.h>
#include <Marco/MScreen.h>
#include <Marco/roles/MLayerSurface.h>
#include <AK/nodes/AKText.h>
#include <AK/nodes/AKButton.h>
#include <AK/AKLog.h>
#include <AK/nodes/AKImageFrame.h>
#include <AK/utils/AKImageLoader.h>
#include <AK/AKAnimation.h>

#include <DockContainer.h>
#include <Theme.h>

using namespace AK;

static constexpr AKBitset<AKEdge> anchors[] {
    AKEdgeLeft,
    AKEdgeTop,
    AKEdgeRight,
    AKEdgeBottom
};

class Window : public MLayerSurface
{
public:
    Window(Layer layer, AKBitset<AKEdge> anchor, Int32 exclusiveZone, MScreen *screen, const std::string &scope) noexcept
        : MLayerSurface(layer, anchor, exclusiveZone, screen, scope)
    {
        logo = AKImageLoader::loadFile("/usr/local/share/Kay/assets/logo.png");

        setColorWithAlpha(SK_ColorTRANSPARENT);
        exitButton.on.clicked.subscribe(this, [](){ exit(0); });
        exitButton.layout().setWidth(60);

        addButton.on.clicked.subscribe(this, [this](){
            items.emplace_back(std::make_unique<AKImageFrame>(logo, &container));
            items.back()->layout().setAspectRatio(1.f);
            items.back()->layout().setHeightPercent(1.f);
            items.back()->setSizeMode(AKImageFrame::SizeMode::Contain);
            AKImageFrame *frame { items.back().get() };
            AKAnimation::OneShot(300, [this, frame](AKAnimation *anim){
                frame->layout().setHeightPercent(75.f * (1.f - (1.f - SkScalarPow(anim->value(), 2.f))) );
                layout().calculate();
                layout().setWidth(container.layout().calculatedWidth());
                update();
            });
        });
        addButton.layout().setWidth(60);

        anchorButton.on.clicked.subscribe(this, [this](){
            if (anchorI == 1)
                setMargin({0, -50, 0, 0});
            else
                setMargin({0, 0, 0, 0});
            setAnchor(anchors[anchorI]);
            if (anchorI == 3) anchorI = 0;
            else anchorI++;
        });

        layout().setHeight(Theme::DockHeight + 2 * Theme::DockShadowRadius);
        layout().setWidth(screen->props().modes.front().size.width() / screen->props().scale);
        layout().setAlignItems(YGAlignCenter);
        show();
    }

    size_t anchorI { 0 };
    sk_sp<SkImage> logo;
    DockContainer container { this };
    std::vector<std::unique_ptr<AKImageFrame>> items;
    AKButton exitButton { "Exit", &container };
    AKButton anchorButton { "Anchor", &container };
    AKButton addButton { "Add", &container };
};

int main()
{
    setenv("KAY_DEBUG", "4", 0);
    MApplication app;
    setTheme(new Theme());
    app.setAppId("org.Cuarzo.marco-basic");

    if (app.screens().empty())
    {
        AKLog::fatal("No screens available!");
        exit(1);
    }

    std::vector<std::unique_ptr<Window>> windows;

    for (MScreen *screen : app.screens())
    {
        windows.emplace_back(std::make_unique<Window>(
            Window::Overlay,
            AKEdgeLeft | AKEdgeBottom | AKEdgeRight,
            0,
            screen,
            "Marco"));
    }

    return app.exec();
}
