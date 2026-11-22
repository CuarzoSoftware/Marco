#ifndef MDIALOG_H
#define MDIALOG_H

#include <CZ/Marco/Roles/MToplevel.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKText.h>
#include <CZ/AK/Nodes/AKButton.h>
#include <CZ/AK/Nodes/AKIconTile.h>
#include <CZ/AK/Nodes/AKCoreTextEditor.h>
#include <functional>
#include <memory>
#include <vector>

/**
 * @brief A dialog window attached to a parent window (like macOS's sheets and alerts).
 *
 * [icon (optional)]
 * [title (bold)]
 * [message (wrapped, optional)]
 * [content: the app's controls, e.g. a password field]
 * [buttons, right-aligned: the default one highlighted, the others plain]
 *
 * Its background is blurred (vibrancy, the Popover material), or the window background color
 * where the compositor doesn't blur.
 *
 * Open it over a window with open(). Buttons added with addButton() close it after running their
 * action (unless it returns false). accept() and reject() trigger the default and cancel buttons
 * (e.g. from a text field's Enter and Escape); closing it from the compositor rejects it.
 */
class CZ::MDialog : public MToplevel
{
public:
    /** @brief What a button does besides its action. */
    enum class Role
    {
        Normal,  ///< Just its action
        Default, ///< Highlighted, triggered by accept()
        Cancel   ///< Triggered by reject() (and closing the window)
    };

    MDialog(const std::string &title = "", const std::string &message = "") noexcept;
    ~MDialog();

    static constexpr SkScalar Width   { 300.f };
    static constexpr SkScalar Padding { 20.f };

    void setDialogTitle(const std::string &text) noexcept;
    void setMessage(const std::string &text) noexcept;

    /** @brief Shows the icon above the title (empty: hidden). */
    void setIcon(const std::string &iconName, CZAdaptiveColor tileColor) noexcept;

    /**
     * @brief Adds a button (owned by the dialog, laid out in order, right-aligned).
     *
     * @param action Run when clicked. Returning false keeps the dialog open (e.g. invalid input).
     */
    AKButton &addButton(const std::string &text, Role role = Role::Normal, std::function<bool()> action = {}) noexcept;

    /**
     * @brief Keeps @p node alive as long as the dialog (e.g. rows created for its content: nodes
     *        don't own their children).
     */
    template <class T>
    T &own(std::unique_ptr<T> node) noexcept
    {
        T &ref { *node };
        m_owned.emplace_back(std::move(node));
        return ref;
    }

    /** @brief Opens it over @p parent. */
    void open(MToplevel &parent) noexcept;

    /** @brief Closes it (no button action runs). */
    void close() noexcept;

    /** @brief Triggers the default button. */
    void accept() noexcept;

    /** @brief Triggers the cancel button (or just closes it). */
    void reject() noexcept;

    MVibrancyView background { this };   ///< Behind everything (first child)
    AKContainer body { YGFlexDirectionColumn, false, this };
    AKIconTile icon { "", CZAdaptiveColor(0), 48, &body };
    AKCoreTextEditor title { "", &body };   ///< Wrapped
    AKCoreTextEditor message { "", &body };
    AKContainer content { YGFlexDirectionColumn, false, &body };
    AKContainer buttons { YGFlexDirectionRow, false, &body };

protected:
    void layoutEvent(const CZLayoutEvent &e) override;

private:
    struct Button
    {
        std::unique_ptr<AKButton> button;
        Role role;
        std::function<bool()> action;
    };

    void trigger(Button &button) noexcept;
    std::vector<std::unique_ptr<Button>> m_buttons;
    std::vector<std::unique_ptr<AKNode>> m_owned;
};

#endif // MDIALOG_H
