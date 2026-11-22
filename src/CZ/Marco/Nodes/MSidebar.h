#ifndef MSIDEBAR_H
#define MSIDEBAR_H

#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/AK/Nodes/AKWindowButtonGroup.h>
#include <CZ/Core/CZSignal.h>

/**
 * @brief A macOS-style window sidebar: vibrancy background, window buttons and a resizable width.
 *
 * Meant to be the first child of a row-direction toplevel (e.g. an MToplevel with
 * `YGFlexDirectionRow`), followed by the window's main content (usually with `flex: 1`):
 *
 * @code
 * MToplevel window;
 * window.layout().setFlexDirection(YGFlexDirectionRow);
 * MSidebar sidebar { &window };
 * AKSolidColor content { AKTheme::WindowBackground, &window };
 * content.layout().setFlex(1.f);
 * @endcode
 *
 * It's a column with:
 *
 * - header: a row the window is dragged by (UCWindowMove), holding the window control buttons
 *   (windowButtons, the "traffic lights") at its left. Other nodes can be added after them.
 * - content: everything below it (flex: 1), where the app adds its items (search field, list...).
 *
 * The window buttons are handled by MToplevel (they perform their actions and are hidden while
 * fullscreen or server-side decorated).
 *
 * The user can resize it by dragging its trailing edge, between minWidth() and maxWidth(). It never
 * grows past what its siblings leave free (their minimum widths), so resizing it never resizes the
 * window.
 */
class CZ::MSidebar : public MVibrancyView
{
public:
    /** @brief Creates the sidebar (DefaultWidth wide), added to @p parent. */
    MSidebar(AKNode *parent = nullptr) noexcept;

    /// @name Default metrics
    /// @{
    static constexpr SkScalar DefaultWidth      { 240.f };
    static constexpr SkScalar DefaultMinWidth   { 180.f };
    static constexpr SkScalar DefaultMaxWidth   { 400.f };
    static constexpr SkScalar HeaderHeight      { 52.f };  ///< Height of the header row
    static constexpr SkScalar HeaderPadding     { 20.f };  ///< Left padding of the window buttons
    static constexpr SkScalar ResizeHandleWidth { 6.f };   ///< Grabbable width at the trailing edge
    /// @}

    /**
     * @brief Sets the width (clamped to minWidth()..maxWidth()).
     *
     * Emits onWidthChanged if it changes.
     */
    void setWidth(SkScalar width) noexcept;

    /** @brief The width set with setWidth() or by the user (the laid out one may be smaller). */
    SkScalar width() const noexcept { return m_width; }

    /** @brief Sets the range the width can take (also when resized by the user). */
    void setWidthRange(SkScalar minWidth, SkScalar maxWidth) noexcept;
    SkScalar minWidth() const noexcept { return m_minWidth; }
    SkScalar maxWidth() const noexcept { return m_maxWidth; }

    /** @brief Whether the user can resize it by dragging its trailing edge (true by default). */
    void setResizable(bool resizable) noexcept;
    bool resizable() const noexcept { return m_resizable; }

    /** @brief Whether the user is resizing it right now. */
    bool resizing() const noexcept { return m_handle.m_resizing; }

    /** @brief Emitted after width() changes (e.g. to save it). */
    CZSignal<SkScalar> onWidthChanged;

    /** @brief Row at the top the window is dragged by, with the window buttons. */
    AKContainer header { YGFlexDirectionRow, false, this };

    /** @brief The window control buttons, at the left of the header. */
    AKWindowButtonGroup windowButtons { &header };

    /** @brief The area below the header (a column, flex: 1), for the app's items. */
    AKContainer content { YGFlexDirectionColumn, false, this };

protected:
    /// @cond INTERNAL
    class ResizeHandle : public AKContainer
    {
    public:
        ResizeHandle(MSidebar &sidebar) noexcept;
        void pointerButtonEvent(const CZPointerButtonEvent &e) override;
        void pointerMoveEvent(const CZPointerMoveEvent &e) override;
        void stop() noexcept;
        MSidebar &m_sidebar;
        SkScalar m_startWidth {}, m_startX {};
        bool m_resizing { false };
    };

    /// The widest it can be now: maxWidth(), or less if its siblings need the room.
    SkScalar availableMaxWidth() const noexcept;

    ResizeHandle m_handle { *this };
    SkScalar m_width { DefaultWidth };
    SkScalar m_minWidth { DefaultMinWidth };
    SkScalar m_maxWidth { DefaultMaxWidth };
    bool m_resizable { true };
    /// @endcond
};

#endif // MSIDEBAR_H
