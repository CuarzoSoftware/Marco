#ifndef MMENU_H
#define MMENU_H

#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Marco/Nodes/MMenuView.h>
#include <CZ/Marco/Nodes/MMenuAction.h>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief A popup menu with a vibrancy background and rounded corners, like macOS's.
 *
 * Its content is an MMenuView (view, the slot). Build it with addAction() and addDivider() (owned
 * by the menu, see clear()) and open it under a node with popup(). Activating an item closes it.
 *
 * @code
 * MMenu menu;
 * menu.addAction("Auto-Join", []{ ... }).setChecked(true);
 * menu.addDivider();
 * menu.addAction("Forget This Network…", []{ ... });
 * menu.popup(button, &event); // e.g. from the button's click
 * @endcode
 */
class CZ::MMenu : public MPopup
{
public:
    MMenu() noexcept;
    ~MMenu();

    static constexpr Int32 Padding { 5 }; ///< Around the items

    /** @brief Adds an item (owned by the menu) running @p onClicked when activated. */
    MMenuAction &addAction(const std::string &text, std::function<void()> onClicked = {}, bool enabled = true) noexcept;

    /** @brief Adds a divider line (owned by the menu). */
    void addDivider() noexcept;

    /** @brief Removes the items added with addAction() and addDivider(). */
    void clear() noexcept;

    /**
     * @brief Opens it below @p anchor (a node of another surface), left-aligned with it (flipped or
     *        slid if there's no room).
     *
     * @param grab The input event that opened it (e.g. a button press), for the popup grab.
     */
    void popup(const AKNode &anchor, const CZInputEvent *grab = nullptr) noexcept;

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond

protected:
    MVibrancyView m_vibrancy { this };

public:
    /** @brief The items (the slot: nodes added to the menu go here). */
    MMenuView view { this };

private:
    std::vector<std::unique_ptr<AKNode>> m_items;
    std::unique_ptr<Imp> m_imp;
};

#endif // MMENU_H
