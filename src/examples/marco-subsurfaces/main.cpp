#include <AK/AKLog.h>
#include <Marco/MApplication.h>
#include <Marco/roles/MToplevel.h>
#include <Marco/roles/MSubSurface.h>
#include <AK/nodes/AKButton.h>
#include <AK/nodes/AKText.h>
#include <AK/AKAnimation.h>

using namespace AK;

class SubWindow : public MSubSurface
{
public:
    SubWindow(MSurface *parent, Int32 size, size_t n) :
        MSubSurface(parent),
        n(n),
        label {std::to_string(n), this }
    {
        auto style = label.textStyle();
        SkPaint p;
        p.setColor(SK_ColorWHITE);
        style.setForegroundColor(p);
        label.setTextStyle(style);
        setColorWithoutAlpha(SkColorSetRGB(rand() % 255, rand() % 255, rand() % 255));
        layout().setWidth(size);
        layout().setHeight(size);
        layout().setJustifyContent(YGJustifyCenter);
        layout().setAlignItems(YGAlignCenter);
        setPos({200, 200});
        setMapped(true);
    }

    size_t n;
    AKText label;
};

class Window : public MToplevel
{
public:
    Window() noexcept : MToplevel()
    {
        layout().setPadding(YGEdgeAll, 32.f);
        layout().setGap(YGGutterAll, 32.f);

        addButton.on.clicked.subscribe(this, [this](){
            new SubWindow(this, 48, subSurfaces().size());
        });

        animateButton.on.clicked.subscribe(this, [this](){
            animated = !animated;

            if (animated)
                spinAnimation.start();
            else
                spinAnimation.stop();
        });

        exitButton.on.clicked.subscribe(this, [](){exit(0);});

        spinAnimation.setDuration(10000);
        spinAnimation.setOnUpdateCallback([this](AKAnimation *a){

            const SkScalar phase =  2.f * M_PI * a->value();
            const SkScalar subN = subSurfaces().size();
            const SkScalar phaseSlice = (2.f * M_PI)/subN;
            const SkSize tlSize (surfaceSize().width(), surfaceSize().height());
            const SkSize tlCenter = { tlSize.width() / 2.f, tlSize.height() / 2.f };
            const SkSize radius (tlCenter.width() + 100.f, tlCenter.height() + 100.f );

            SkScalar x, y;
            for (MSubSurface *subS : subSurfaces())
            {
                SubWindow *s = (SubWindow*)subS;
                x = SkScalarCos(phaseSlice * s->n + phase);
                y = SkScalarSin(phaseSlice * s->n + phase);

                s->setPos(tlCenter.width() + x * radius.width() - s->globalRect().width() / 2,
                          tlCenter.height() + y * radius.height() - s->globalRect().height() / 2);
            }

        });

        spinAnimation.setOnFinishCallback([this](AKAnimation *a){
            if (animated)
                a->start();
        });
    }

    bool animated { false };
    AKAnimation spinAnimation;
    AKButton addButton { "Create subsurface", this };
    AKButton animateButton { "Animate subsurfaces", this };
    AKButton exitButton { "Exit", this };
};

int main()
{
    setenv("KAY_DEBUG", "4", 1);
    MApplication app;
    app.setAppId("org.Cuarzo.marco-subsurfaces");

    Window window;
    window.setMapped(true);
    return app.exec();
}
