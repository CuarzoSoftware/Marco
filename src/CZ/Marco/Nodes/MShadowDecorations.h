#ifndef MSHADOWDECORATIONS_H
#define MSHADOWDECORATIONS_H

#include <CZ/Marco/Nodes/MDecorations.h>
#include <CZ/Marco/MTheme.h>
#include <CZ/AK/Nodes/AKImage.h>
#include <CZ/skia/core/SkPoint.h>

/**
 * @brief Default client-side decorations: a drop shadow plus rounded content corners.
 *
 * The shadow is fully described by a blur @ref radius() and an @ref offset() (x, y). These are
 * plain, settable parameters: the node does not inspect the surface's activation state itself.
 * A role decides the look - e.g. MToplevel sets a larger radius/offset while active and a smaller
 * one while inactive, whereas MPopup leaves them fixed.
 *
 * The node draws nothing itself; it holds, as children rendered in order, the 4 rounded-corner
 * masks (RBlendMode::DstIn) followed by the drop-shadow node. That ordering is required so the
 * render sequence is: window content -> corner masks -> shadow (otherwise the DstIn masks would
 * carve into the already-drawn shadow).
 */
class CZ::MShadowDecorations : public MDecorations
{
public:
    MShadowDecorations() noexcept;

    /** @brief Sets the shadow blur radius (in logical pixels). */
    void setRadius(Int32 radius) noexcept;
    Int32 radius() const noexcept { return m_radius; }

    /** @brief Sets the shadow offset. A positive x/y shifts the shadow right/down. */
    void setOffset(const SkIPoint &offset) noexcept;
    const SkIPoint &offset() const noexcept { return m_offset; }

    void subtractOpaque(SkRegion &opaque) const noexcept override;

protected:
    // Draws the theme drop shadow. Kept as the last child so it renders after the corner masks.
    class ShadowNode : public AKRenderable
    {
    public:
        ShadowNode(MShadowDecorations *deco) noexcept;
        // Rebuilds the shadow image from the current node size and the decoration's radius/offset.
        void rebuild() noexcept;
    protected:
        void layoutEvent(const CZLayoutEvent &e) override;
        void renderEvent(const AKRenderEvent &e) override;
        MShadowDecorations *m_deco;
        std::shared_ptr<RImage> m_image;
        CZBitset<MTheme::ShadowClamp> m_clampSides;
    };

    void onBind() noexcept override;
    void updateMargins() noexcept;

    // Recomputes the shadow's invisible region: the central widget area minus the rounded corners,
    // which is fully covered by the opaque content and therefore needs no shadow behind it.
    void updateInvisibleRegion() noexcept;

    Int32 m_radius { MTheme::CSDShadowActiveRadius };
    SkIPoint m_offset { 0, MTheme::CSDShadowActiveOffsetY };
    AKImage m_cornerRadius[4]; // Rounded-corner masks (DstIn), rendered before the shadow.
    ShadowNode m_shadow;
};

#endif // MSHADOWDECORATIONS_H
