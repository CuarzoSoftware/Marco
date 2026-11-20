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
    /** @brief Creates an action with the given title, added to @p parent (usually an MMenuView). */
    MMenuAction(const std::string &text = "", AKNode *parent = nullptr) noexcept;

    /** @brief Sets the title. */
    void setText(const std::string &text) noexcept;

    /** @brief Sets the icon shown before the title (`nullptr` removes it). */
    void setIcon(std::shared_ptr<RImage> icon) noexcept;

    /** @brief Sets the shortcut label shown at the end, e.g. "Ctrl+C" (empty removes it). Display only. */
    void setShortcut(const std::string &shortcut) noexcept;

    /** @brief Enables or disables the action (disabled actions are dimmed and can't be activated). */
    void setEnabled(bool enabled) noexcept;
    bool enabled() const noexcept override { return m_enabled; }

    /// Shows a check mark in the leading gutter (see MMenuView::setCheckGutter()).
    void setChecked(bool checked) noexcept;

    /** @brief Whether the check mark is shown, see setChecked(). */
    bool checked() const noexcept { return m_checked; }

    void setHover(bool hover) noexcept override;
    void setCheckGutterVisible(bool visible) noexcept override;

    AKPath check;                       ///< The check mark.
    std::unique_ptr<AKImageFrame> icon; ///< The icon, `nullptr` if not set (see setIcon()).
    AKText text;                        ///< The title.
    AKContainer spacer;                 ///< Flexible space between the title and the shortcut.
    std::unique_ptr<AKText> shortcut;   ///< The shortcut label, `nullptr` if not set (see setShortcut()).

private:
    void applyColors() noexcept;
    bool m_hover { false };
    bool m_enabled { true };
    bool m_checked { false };
    bool m_gutter { false };
};

#endif // MMENUACTION_H
