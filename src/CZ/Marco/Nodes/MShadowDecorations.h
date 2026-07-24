#ifndef MSHADOWDECORATIONS_H
#define MSHADOWDECORATIONS_H

#include <CZ/Marco/Nodes/MDecorations.h>
#include <CZ/Marco/MTheme.h>
#include <CZ/AK/Nodes/AKImage.h>

/**
 * @brief Default client-side decorations: a drop shadow plus rounded content corners.
 *
 * This node draws nothing itself; it holds, as children rendered in order, the 4 rounded-corner
 * masks (RBlendMode::DstIn) followed by the drop-shadow node. That ordering is required so the
 * render sequence is: window content -> corner masks -> shadow (otherwise the DstIn masks would
 * carve into the already-drawn shadow).
 */
class CZ::MShadowDecorations : public MDecorations
{
public:
    MShadowDecorations() noexcept;
    void subtractOpaque(SkRegion &opaque) const noexcept override;

protected:
    // Draws the theme drop shadow. Kept as the last child so it renders after the corner masks.
    class ShadowNode : public AKRenderable
    {
    public:
        ShadowNode(MShadowDecorations *deco) noexcept;
    protected:
        void layoutEvent(const CZLayoutEvent &e) override;
        void renderEvent(const AKRenderEvent &e) override;
        MShadowDecorations *m_deco;
        std::shared_ptr<RImage> m_image;
        CZBitset<MTheme::ShadowClamp> m_clampSides;
    };

    void onBind() noexcept override;
    void windowStateEvent(const CZWindowStateEvent &event) override;
    void updateMargins() noexcept;

    // Recomputes the shadow's invisible region: the central widget area minus the rounded corners,
    // which is fully covered by the opaque content and therefore needs no shadow behind it.
    void updateInvisibleRegion() noexcept;

    // Cached activation state the current margins/shadow were computed from. The shadow slicing
    // must use this (not a live activated() read) so the margins and the shadow always agree,
    // even on the first frame before the compositor has reported the real activation state.
    bool m_active { true };
    AKImage m_cornerRadius[4]; // Rounded-corner masks (DstIn), rendered before the shadow.
    ShadowNode m_shadow;
};

#endif // MSHADOWDECORATIONS_H
