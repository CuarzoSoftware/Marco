#ifndef MTOOLTIP_H
#define MTOOLTIP_H

#include <CZ/Marco/Roles/MPopup.h>
#include <CZ/Marco/Nodes/MVibrancyView.h>
#include <CZ/Marco/Nodes/MDecorations.h>
#include <CZ/AK/Nodes/AKBakeable.h>
#include <CZ/AK/Nodes/AKContainer.h>
#include <CZ/Core/CZEdge.h>
#include <CZ/skia/core/SkPath.h>

/**
 * @brief A popup with a macOS-style rounded caret pointing at a given point of its parent.
 *
 * Like MMenu, but its outline is a single recorded path (a rounded body plus the caret on one of
 * its edges). That same path is used to:
 *  - draw the tooltip: the fill (only when background blur isn't available) and the hairline borders,
 *  - generate its drop shadow (the decorations),
 *  - mask the background blur (MSurface::setBlurMaskPath()).
 *
 * Content goes into the tooltip itself (it's the slot, padded away from the caret). The caret
 * keeps pointing at the target when the compositor slides the popup (e.g. near a screen edge), and
 * moves to the opposite edge if it flips it.
 */
class CZ::MTooltip : public MPopup
{
public:
    static constexpr Int32 CaretWidth   { 28 }; // Base width, including the flared base
    static constexpr Int32 CaretHeight  { 11 };
    static constexpr Int32 BorderRadius { 10 };
    static constexpr Int32 Padding      { 5 };
    static constexpr Int32 ShadowRadius { 24 };
    static constexpr Int32 ShadowOffsetY { 6 };

    MTooltip() noexcept;

    /**
     * @brief Positions the tooltip so its caret points at @p point.
     *
     * @param parent The surface the tooltip is attached to.
     * @param point Target in @p parent's surface-local coordinates.
     * @param caretEdge The tooltip edge holding the caret (e.g. CZEdgeBottom places the tooltip
     *                  above the point). The compositor may flip it.
     */
    void pointTo(MSurface *parent, const SkIPoint &point, CZEdge caretEdge = CZEdgeBottom) noexcept;

    /// The target point (parent coordinates).
    const SkIPoint &target() const noexcept { return m_target; }

    /// The edge the caret is currently on (the requested one, or the opposite if flipped).
    CZEdge caretEdge() const noexcept { return m_caretEdge; }

    /// The tooltip outline, in central-node-local coordinates.
    const SkPath &shape() const noexcept { return m_shape; }

    /// Emitted after shape() changes.
    CZSignal<> onShapeChanged;

    /// Fill used when background blur isn't available (or is disabled).
    void setFallbackColor(CZAdaptiveColor color) noexcept;

    /**
     * @brief Uses the compositor's background blur behind the tooltip (default) or the fallback fill.
     *
     * The compositor can't fade the blur, so disable it for tooltips animated with setFadeOpacity().
     */
    void setBlurEnabled(bool enabled) noexcept;
    bool blurEnabled() const noexcept { return m_blurEnabled; }

    /**
     * @brief Fades the whole tooltip: fill, borders, shadow and every renderable in its content.
     *
     * Overrides the opacity of the content's renderables. Call it again after adding content.
     */
    void setFadeOpacity(SkScalar opacity) noexcept;
    SkScalar fadeOpacity() const noexcept { return m_fadeOpacity; }

protected:
    void assignedRectChanged() override;
    void layoutEvent(const CZLayoutEvent &event) override;

private:
    /// Draws the fill and the hairline borders along shape().
    class ShapeNode final : public AKBakeable
    {
    public:
        ShapeNode(MTooltip &tooltip) noexcept;
        void rebake() noexcept;
    protected:
        void bakeEvent(const AKBakeEvent &event) override;
    private:
        MTooltip &m_tooltip;
    };

    /// Drop shadow generated from shape().
    class Shadow final : public MDecorations
    {
    public:
        Shadow(MTooltip &tooltip) noexcept;
        void onBind() noexcept override;
        void rebake() noexcept { m_node.rebake(); }
        void setShadowOpacity(SkScalar opacity) noexcept { m_node.setOpacity(opacity); }
    private:
        class Node final : public AKBakeable
        {
        public:
            Node(MTooltip &tooltip, Shadow &shadow) noexcept;
            void rebake() noexcept;
        protected:
            void bakeEvent(const AKBakeEvent &event) override;
        private:
            MTooltip &m_tooltip;
            Shadow &m_shadow;
        } m_node;
    };

    void updateCaret() noexcept;
    void updateShape() noexcept;
    void updatePadding() noexcept;

    MVibrancyView m_vibrancy;
    ShapeNode m_shapeNode { *this };
    AKContainer m_content;

    SkIPoint m_target {};
    CZEdge m_requestedEdge { CZEdgeBottom };
    CZEdge m_caretEdge { CZEdgeBottom };
    Float32 m_caretOffset { -1.f }; // Along the caret edge, from its start (-1 = centered)
    SkPath m_shape;
    SkISize m_shapeSize {};
    CZAdaptiveColor m_fallbackColor { 0xFFEDEDED, 0xFF2B2B2B };
    Shadow *m_shadow {};
    bool m_blurEnabled { true };
    SkScalar m_fadeOpacity { 1.f };
};

#endif // MTOOLTIP_H
