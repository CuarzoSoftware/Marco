#include <CZ/Marco/Roles/MDialog.h>
#include <CZ/AK/AKTheme.h>
#include <CZ/Core/Events/CZCloseEvent.h>
#include <CZ/Core/Events/CZLayoutEvent.h>
#include <algorithm>

using namespace CZ;

MDialog::MDialog(const std::string &titleText, const std::string &messageText) noexcept : MToplevel()
{
    // Transparent: the background view shows the blur (or its fallback color)
    setColor(CZAdaptiveColor(0));
    setBlurMaterial(MBlurMaterial::Popover);
    layout().setWidth(Width);

    const SkRegion empty;
    background.setInputRegion(&empty);
    background.setDisabledColor(AKTheme::WindowBackground);
    background.layout().setPositionType(YGPositionTypeAbsolute);
    background.layout().setPosition(YGEdgeLeft, 0.f);
    background.layout().setPosition(YGEdgeTop, 0.f);
    background.layout().setWidthPercent(100.f);
    background.layout().setHeightPercent(100.f);

    body.layout().setPadding(YGEdgeAll, Padding);
    body.layout().setGap(YGGutterRow, 10.f);
    body.layout().setAlignItems(YGAlignStretch);
    body.layout().setWidthPercent(100.f);

    // Dragging the dialog anywhere (but its controls) moves it
    body.userCaps.add(UCWindowMove);

    icon.layout().setAlignSelf(YGAlignFlexStart);
    icon.setVisible(false);

    title.setFontSize(13.f);
    title.setFontStyle(SkFontStyle(SkFontStyle::kBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant));
    title.setTextColor(AKTheme::Label);
    title.setSelectable(false);
    title.setWrapWidth(Width - 2.f * Padding);

    message.setFontSize(11.f);
    message.setTextColor(AKTheme::Label);
    message.setSelectable(false);
    message.setWrapWidth(Width - 2.f * Padding);

    content.layout().setGap(YGGutterRow, 8.f);

    buttons.layout().setJustifyContent(YGJustifyFlexEnd);
    buttons.layout().setGap(YGGutterColumn, 8.f);
    buttons.layout().setMargin(YGEdgeTop, 6.f);

    setDialogTitle(titleText);
    setMessage(messageText);

    // Closed by the compositor (or the user): rejected
    onBeforeClose.subscribe(this, [this](const CZCloseEvent &){ reject(); });
}

MDialog::~MDialog()
{
    m_buttons.clear();
    m_owned.clear();
}

void MDialog::setDialogTitle(const std::string &text) noexcept
{
    title.setText(text);
    title.setVisible(!text.empty());
    setTitle(text);
}

void MDialog::layoutEvent(const CZLayoutEvent &e)
{
    MToplevel::layoutEvent(e);

    // The texts wrap at the dialog's width (it may be changed, e.g. layout().setWidth())
    const SkScalar wrap { std::max(1.f, layout().calculatedWidth() - 2.f * Padding) };

    if (title.wrapWidth() != wrap)
    {
        title.setWrapWidth(wrap);
        message.setWrapWidth(wrap);
    }
}

void MDialog::setMessage(const std::string &text) noexcept
{
    message.setText(text);
    message.setVisible(!text.empty());
}

void MDialog::setIcon(const std::string &iconName, CZAdaptiveColor tileColor) noexcept
{
    icon.icon.setIconFromName(iconName);
    icon.setTileColor(tileColor);
    icon.setVisible(!iconName.empty());
}

AKButton &MDialog::addButton(const std::string &text, Role role, std::function<bool()> action) noexcept
{
    auto &b { m_buttons.emplace_back(std::make_unique<Button>()) };
    b->button = std::make_unique<AKButton>(text, &buttons);
    b->role = role;
    b->action = std::move(action);
    b->button->layout().setMinWidth(72.f);

    // The default one with the accent color, the others plain (like macOS)
    if (role == Role::Default)
    {
        b->button->setBackgroundColor(AKTheme::ControlAccent);
        b->button->setContentColor(AKTheme::White);
    }
    else
    {
        b->button->setBackgroundColor(CZAdaptiveColor(0x1F000000, 0x2EFFFFFF)); // Translucent: visible over the blur
        b->button->setContentColor(AKTheme::ControlText);
    }

    Button *raw { b.get() };
    b->button->onClick.subscribe(this, [this, raw](const auto &){ trigger(*raw); });
    return *raw->button;
}

void MDialog::open(MToplevel &parent) noexcept
{
    setParentToplevel(&parent);
    setMapped(true);
}

void MDialog::close() noexcept
{
    setMapped(false);
}

void MDialog::trigger(Button &button) noexcept
{
    if (button.action && !button.action())
        return;

    close();
}

void MDialog::accept() noexcept
{
    for (auto &b : m_buttons)
        if (b->role == Role::Default && b->button->enabled())
            return trigger(*b);
}

void MDialog::reject() noexcept
{
    for (auto &b : m_buttons)
        if (b->role == Role::Cancel)
            return trigger(*b);

    close();
}
