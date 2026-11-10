#ifndef MMENUACTION_H
#define MMENUACTION_H

#include <CZ/Marco/Nodes/MMenuItem.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/AK/Nodes/AKImageFrame.h>
#include <CZ/AK/Nodes/AKPath.h>
#include <memory>

/**
 * @brief A clickable menu row: optional check mark, optional icon, title and optional shortcut.
 */
class CZ::MMenuAction : public MMenuItem
{
public:
    MMenuAction(const std::string &text = "", AKNode *parent = nullptr) noexcept;

    void setText(const std::string &text) noexcept;
    void setIcon(std::shared_ptr<RImage> icon) noexcept;
    void setShortcut(const std::string &shortcut) noexcept;

    void setEnabled(bool enabled) noexcept;
    bool enabled() const noexcept override { return m_enabled; }

    /// Shows a check mark in the leading gutter (see MMenuView::setCheckGutter()).
    void setChecked(bool checked) noexcept;
    bool checked() const noexcept { return m_checked; }

    void setHover(bool hover) noexcept override;
    void setCheckGutterVisible(bool visible) noexcept override;

    AKPath check;
    std::unique_ptr<AKImageFrame> icon;
    AKText text;
    AKContainer spacer;
    std::unique_ptr<AKText> shortcut;

private:
    void applyColors() noexcept;
    bool m_hover { false };
    bool m_enabled { true };
    bool m_checked { false };
    bool m_gutter { false };
};

#endif // MMENUACTION_H
