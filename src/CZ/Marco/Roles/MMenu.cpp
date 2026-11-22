#include <CZ/Marco/Roles/MMenu.h>
#include <CZ/Marco/Private/MMenuPrivate.h>
#include <CZ/Marco/Nodes/MShadowDecorations.h>
#include <CZ/Marco/Nodes/MMenuDivider.h>
#include <CZ/AK/AKScene.h>

using namespace CZ;

MMenu::MMenu() noexcept : MPopup()
{
    setBlurMaterial(MBlurMaterial::Menu);
    setColor(CZAdaptiveColor(0));
    setDecorations(std::make_unique<MShadowDecorations>());
    setBorderRadius(CZBorderRadius::Make(8));

    // The blur behind everything, the items (the slot) sizing the menu
    const SkRegion empty;
    m_vibrancy.setInputRegion(&empty);
    m_vibrancy.layout().setPositionType(YGPositionTypeAbsolute);
    m_vibrancy.layout().setPosition(YGEdgeLeft, 0.f);
    m_vibrancy.layout().setPosition(YGEdgeTop, 0.f);
    m_vibrancy.layout().setWidthPercent(100.f);
    m_vibrancy.layout().setHeightPercent(100.f);

    view.layout().setPadding(YGEdgeAll, Padding);
    view.setCheckGutter(true);
    setSlot(&view);

    view.onItemActivated.subscribe(this, [this](MMenuItem*){ setMapped(false); });
}

MMenu::~MMenu()
{
    m_items.clear();
}

MMenu::Imp *MMenu::imp() const noexcept
{
    return m_imp.get();
}

MMenuAction &MMenu::addAction(const std::string &text, std::function<void()> onClicked, bool enabled) noexcept
{
    auto action { std::make_unique<MMenuAction>(text, &view) };
    action->onClicked = std::move(onClicked);
    action->setEnabled(enabled);
    MMenuAction &ref { *action };
    m_items.emplace_back(std::move(action));
    return ref;
}

void MMenu::addDivider() noexcept
{
    m_items.emplace_back(std::make_unique<MMenuDivider>(&view));
}

void MMenu::clear() noexcept
{
    m_items.clear();
}

void MMenu::popup(const AKNode &anchor, const CZInputEvent *grab) noexcept
{
    MSurface *parent { anchor.scene() ? anchor.scene()->window() : nullptr };

    if (!parent)
        return;

    setMapped(false);
    setParent(parent);

    // In the parent's central node coordinates
    SkIRect rect { anchor.worldRect() };
    rect.offset(-parent->worldRect().x(), -parent->worldRect().y());
    setAnchorRect(rect);
    setAnchor(Anchor::BottomLeft);
    setGravity(Gravity::BottomRight);
    setConstraintAdjustment(SlideX | SlideY | FlipX | FlipY);
    setOffset(0, 4);
    setGrab(grab);
    setMapped(true);
}
