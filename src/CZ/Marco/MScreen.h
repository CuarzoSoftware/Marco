#ifndef MSCREEN_H
#define MSCREEN_H

#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZTransform.h>
#include <CZ/Core/CZBitset.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Marco/MProxy.h>
#include <CZ/AK/AKObject.h>
#include <CZ/Ream/RSubpixel.h>
#include <CZ/skia/core/SkSize.h>
#include <CZ/skia/core/SkPoint.h>
#include <optional>

/**
 * @brief A compositor screen (wl_output).
 *
 * Screens are created and destroyed by MApp, see MApp::screens(), MApp::onScreenPlugged and
 * MApp::onScreenUnplugged.
 */
class CZ::MScreen : public AKObject
{
public:
    /** @brief Props that changed, see onPropsChanged. */
    enum Changes
    {
        Position        = 1 << 0, ///< Props::pos
        PhysicalSize    = 1 << 1, ///< Props::physicalSize
        PixelGeometry   = 1 << 2, ///< Props::pixelGeometry
        Scale           = 1 << 3, ///< Props::scale
        Name            = 1 << 4, ///< Props::name
        Description     = 1 << 5, ///< Props::description
        Make            = 1 << 6, ///< Props::make
        Model           = 1 << 7, ///< Props::model
        Transform       = 1 << 8, ///< Props::transform
        Modes           = 1 << 9, ///< Props::modes
        LogicalPosition = 1 << 10, ///< Props::logicalPos
        LogicalSize     = 1 << 11  ///< Props::logicalSize
    };

    /** @brief A display mode (wl_output.mode). */
    struct Mode
    {
        SkISize size;     ///< Size in buffer pixels.
        Int32 refresh;    ///< Refresh rate in mHz.
        bool isCurrent;   ///< Whether it's the current mode.
        bool isPreferred; ///< Whether it's the preferred mode.
    };

    /** @brief The screen properties, as sent by the compositor. */
    struct Props
    {
        std::string name;          ///< Name (e.g. `DP-1`).
        std::string description;   ///< Human-readable description.
        std::string make;          ///< Manufacturer.
        std::string model;         ///< Model.
        SkIPoint pos;              ///< Position in the global compositor space.
        SkISize physicalSize;      ///< Physical size in millimeters.
        Int32 scale { 1 };         ///< Buffer scale factor.
        RSubpixel pixelGeometry;   ///< Subpixel layout.
        CZTransform transform;     ///< Transform applied to the screen content.
        std::vector<Mode> modes;   ///< Display modes.
        std::optional<SkIPoint> logicalPos; ///< Position in the global compositor space (xdg-output), unset without it.
        std::optional<SkISize> logicalSize; ///< Size in surface coordinates, fractional scaling included (xdg-output), unset without it.
    };

    /** @brief The current properties. */
    const Props &props() const noexcept { return m_current; }

    /**
     * @brief The size surfaces get on this screen, in surface coordinates.
     *
     * Props::logicalSize (xdg-output), right with fractional scaling too. Without it (unset: the
     * compositor lacks xdg-output, or hasn't sent it yet), the current mode's size divided by
     * Props::scale (transform applied), empty if there's no current mode.
     */
    SkISize logicalSize() const noexcept;

    /** @brief The wl_output proxy. */
    wl_output *wlOutput() const noexcept { return m_proxy.get(); }

    /** @brief Emitted when one or more props change (atomically), with the changed ones. */
    CZSignal<MScreen&, CZBitset<Changes>> onPropsChanged;
private:
    friend class MApp;
    friend struct MWlRegistry;
    friend struct MWlOutput;
    friend struct MXdgOutput;
    MScreen(void *proxy, UInt32 name) noexcept : m_proxy(proxy, name) {}
    ~MScreen();
    Props m_current, m_pending;
    CZBitset<Changes> m_changes;
    bool m_pendingFirstDone { true };
    MProxy<wl_output> m_proxy;
    zxdg_output_v1 *m_xdgOutput {};
};

#endif // MSCREEN_H
