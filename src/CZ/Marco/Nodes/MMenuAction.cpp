#include <CZ/Marco/Nodes/MMenuAction.h>
#include <CZ/AK/AKTheme.h>

using namespace CZ;

static constexpr SkScalar kFontSize { 13.f };
static constexpr SkScalar kCheckSize { 10.f };
static constexpr SkScalar kIconSize { 16.f };

static SkFontStyle ItemFontStyle() noexcept
{
    return SkFontStyle(SkFontStyle::Weight::kNormal_Weight, SkFontStyle::Width::kNormal_Width, SkFontStyle::Slant::kUpright_Slant);
}

// A filled check mark in a 12x12 box (scaled to the node).
static SkPath CheckPath() noexcept
{
    SkPath path;
    path.moveTo(0.5f, 6.5f);
    path.lineTo(2.f, 5.f);
    path.lineTo(4.5f, 7.5f);
    path.lineTo(10.f, 2.f);
    path.lineTo(11.5f, 3.5f);
    path.lineTo(4.5f, 10.5f);
    path.close();
    return path;
}

MMenuAction::MMenuAction(const std::string &text, AKNode *parent) noexcept :
    MMenuItem(parent),
    check(CheckPath(), this),
    text(text, this),
    spacer(YGFlexDirectionColumn, false, this)
{
    layout().setPadding(YGEdgeTop, 3.f);
    layout().setPadding(YGEdgeBottom, 4.f);
    layout().setPadding(YGEdgeHorizontal, 10.f);
    layout().setGap(YGGutterAll, 6.f);

    SkRegion empty;
    check.setInputRegion(&empty);
    check.setSizeMode(AKPath::ScalePath);
    check.layout().setWidth(kCheckSize);
    check.layout().setHeight(kCheckSize);
    check.setVisible(false);

    this->text.setInputRegion(&empty);
    this->text.setFontSize(kFontSize);
    this->text.setFontStyle(ItemFontStyle());
    spacer.layout().setFlex(10.f);
    onColorSchemeChanged.subscribe(this, [this](CZColorScheme){ applyColors(); });
    applyColors();
    itemReady();
}

void MMenuAction::setText(const std::string &text) noexcept
{
    this->text.setText(text);
}

void MMenuAction::setIcon(std::shared_ptr<RImage> image) noexcept
{
    if (!image)
        return icon.reset();

    if (!icon)
    {
        icon = std::make_unique<AKImageFrame>();
        SkRegion empty;
        icon->setInputRegion(&empty);
        icon->layout().setWidth(kIconSize);
        icon->layout().setHeight(kIconSize);
        icon->setAlignment(CZAlignCenter);
        icon->setSizeMode(AKImageFrame::SizeMode::Contain);
        icon->insertBefore(&text);
    }

    icon->setImage(image);
}

void MMenuAction::setShortcut(const std::string &sc) noexcept
{
    if (sc.empty())
        return shortcut.reset();

    if (!shortcut)
    {
        shortcut = std::make_unique<AKText>(sc, this);
        SkRegion empty;
        shortcut->setInputRegion(&empty);
        shortcut->setFontSize(kFontSize);
        shortcut->setFontStyle(ItemFontStyle());
    }
    else
        shortcut->setText(sc);

    applyColors();
}

void MMenuAction::setEnabled(bool enabled) noexcept
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    applyColors();
}

void MMenuAction::setChecked(bool checked) noexcept
{
    m_checked = checked;
    setCheckGutterVisible(m_gutter);
}

void MMenuAction::setCheckGutterVisible(bool visible) noexcept
{
    m_gutter = visible;

    // The gutter keeps its width even when unchecked, so titles stay aligned.
    check.setVisible(m_gutter || m_checked);
    check.setOpacity(m_checked ? 1.f : 0.f);
}

void MMenuAction::setHover(bool hover) noexcept
{
    if (m_hover == hover)
        return;

    m_hover = hover;
    applyColors();
}

void MMenuAction::applyColors() noexcept
{
    if (m_hover && m_enabled)
    {
        text.setTextColor(CZAdaptiveColor(0xFFFFFFFF));
        if (shortcut) shortcut->setTextColor(CZAdaptiveColor(0xAAFFFFFF));
        check.setColor(0xFFFFFFFF);
    }
    else
    {
        const CZAdaptiveColor label { m_enabled ? AKTheme::Text : AKTheme::TertiaryLabel };
        text.setTextColor(label);
        if (shortcut) shortcut->setTextColor(AKTheme::TertiaryLabel);
        check.setColor(label.resolve(check.colorScheme()));
    }
}
