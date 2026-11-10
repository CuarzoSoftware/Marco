#include <CZ/Marco/Nodes/MMenuDivider.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

MMenuDivider::MMenuDivider(AKNode *parent) noexcept :
    MMenuItem(parent),
    line(AKTheme::TertiaryLabel, this)
{
    layout().setFlexDirection(YGFlexDirectionColumn);
    layout().setAlignItems(YGAlignStretch);
    layout().setMargin(YGEdgeVertical, 4.f);
    layout().setPadding(YGEdgeHorizontal, 10.f);

    SkRegion empty;
    line.setInputRegion(&empty);
    line.layout().setHeight(1.f);
    itemReady();
}
