#ifndef MMENU_H
#define MMENU_H

#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>

/**
 * @brief A popup styled as a menu: translucent blurred background, rounded corners and a drop shadow.
 *
 * An MPopup preconfigured with MBlurMaterial::Menu, a border radius of 8, MShadowDecorations and a
 * transparent color. Children added to it are placed inside an MVibrancyView that fills the menu,
 * so the compositor blurs what's behind it. Position and show it like any MPopup.
 *
 * @code
 * MMenu menu;
 * menu.setParent(&window);
 * menu.setAnchorRect(buttonRect);
 * menu.setAnchor(MPopup::Anchor::BottomLeft);
 * menu.setGravity(MPopup::Gravity::BottomRight);
 * button.onClick.subscribe(&menu, [&menu](const auto &e){
 *     menu.setGrab(&e);
 *     menu.setMapped(true);
 * });
 * @endcode
 */
class CZ::MMenu : public MPopup
{
public:
    /** @brief Creates an unmapped menu. */
    MMenu() noexcept;
    ~MMenu();

    /// @cond INTERNAL
    class Imp;
    Imp *imp() const noexcept;
    /// @endcond

protected:
    /** @brief The node filling the menu that holds its children and requests the background blur. */
    MVibrancyView m_vibrancy { this };
private:
    std::unique_ptr<Imp> m_imp;
};

#endif // MMENU_H
