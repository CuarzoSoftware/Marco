#ifndef MDECORATIONS_H
#define MDECORATIONS_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/Nodes/AKRenderable.h>
#include <CZ/Core/CZWeak.h>
#include <CZ/skia/core/SkRegion.h>

/**
 * @brief Base class for MSurface decorations (shadow, borders, rounded corners, ...).
 *
 * A decoration is a node parented into the surface's root that renders around and over
 * the surface's central node. Subclass it and install it with MSurface::setDecorations();
 * pass `nullptr` for no decorations. The base implementation draws nothing and reserves
 * no margins.
 *
 * @see MShadowDecorations
 */
class CZ::MDecorations : public AKRenderable
{
public:
    MDecorations() noexcept;

    /** @brief The surface this decoration belongs to. */
    MSurface *surface() const noexcept { return m_surface; }

    /**
     * @brief Space (L, T, R, B) the decoration reserves around the central node.
     *
     * MSurface insets the central node by these margins and uses them for the window
     * geometry. Defaults to no margins.
     */
    SkIRect margins() const noexcept { return m_margins; }

    /** @brief Subtracts non-opaque decoration areas (e.g. rounded corners) from @p opaque. */
    virtual void subtractOpaque(SkRegion &opaque) const noexcept { (void)opaque; }

    // Assigned to a surface
    virtual void onBind() noexcept;

    // Unassigned to a surface (surface() still points to it)
    virtual void onUnbind() noexcept;

protected:
    void renderEvent(const AKRenderEvent &event) override;

    // Returns true if changed
    bool setMargins(SkIRect margins) noexcept;

private:
    friend class MSurface;
    void setSurface(MSurface *surface) noexcept;
    CZWeak<MSurface> m_surface;
    SkIRect m_margins { SkIRect::MakeEmpty() };
};

#endif // MDECORATIONS_H
