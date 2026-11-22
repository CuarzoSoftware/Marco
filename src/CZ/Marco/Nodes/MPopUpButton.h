#ifndef MPOPUPBUTTON_H
#define MPOPUPBUTTON_H

#include <CZ/AK/Nodes/AKPopUpButton.h>
#include <CZ/Marco/Roles/MMenu.h>

/**
 * @brief A drop-down list (AKPopUpButton) that shows its items in an MMenu, the selected one checked.
 *
 * @code
 * MPopUpButton rotation { { "Standard", "90°", "180°", "270°" }, &row.trailing };
 * rotation.onSelected.subscribe(this, [](size_t index) { ... });
 * @endcode
 */
class CZ::MPopUpButton : public AKPopUpButton
{
public:
    /** @brief Creates it with @p items, the first one selected (if any). */
    MPopUpButton(const std::vector<std::string> &items = {}, AKNode *parent = nullptr) noexcept;

    /** @brief The menu it opens (rebuilt each time from items()). */
    MMenu menu;

protected:
    void openMenu(const CZPointerButtonEvent &event) noexcept override;
};

#endif // MPOPUPBUTTON_H
