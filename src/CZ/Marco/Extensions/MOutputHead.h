#ifndef MOUTPUTHEAD_H
#define MOUTPUTHEAD_H

#include <CZ/Marco/Protocols/WlrOutputManagement/wlr-output-management-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Marco/MProxy.h>
#include <CZ/AK/AKObject.h>
#include <CZ/Core/CZSignal.h>
#include <CZ/Core/CZBitset.h>
#include <CZ/Core/CZTransform.h>
#include <CZ/skia/core/SkPoint.h>
#include <CZ/skia/core/SkSize.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief An output (a connected display) as wlr-output-management shows it, enabled or not.
 *
 * Listed by MOutputManager (MApp::ext, requires MOutputManagerExt). Created and destroyed by the
 * manager: it's announced with MOutputManager::onAdded once its initial props are known, and
 * destroyed right after MOutputManager::onRemoved (e.g. when the display is unplugged).
 *
 * Unlike MScreen (wl_output), disabled outputs are listed too, along with every mode they support.
 * To change how they're set up, see MOutputManager::apply().
 */
class CZ::MOutputHead : public AKObject
{
public:
    /** @brief Props that changed, see onPropsChanged. */
    enum Changes
    {
        CHName          = 1 << 0,  ///< props().name
        CHDescription   = 1 << 1,  ///< props().description
        CHPhysicalSize  = 1 << 2,  ///< props().physicalSize
        CHModes         = 1 << 3,  ///< props().modes
        CHEnabled       = 1 << 4,  ///< props().enabled
        CHCurrentMode   = 1 << 5,  ///< props().currentMode
        CHPos           = 1 << 6,  ///< props().pos
        CHTransform     = 1 << 7,  ///< props().transform
        CHScale         = 1 << 8,  ///< props().scale
        CHMake          = 1 << 9,  ///< props().make
        CHModel         = 1 << 10, ///< props().model
        CHSerialNumber  = 1 << 11, ///< props().serialNumber
        CHAdaptiveSync  = 1 << 12  ///< props().adaptiveSync
    };

    /** @brief The output properties, as sent by the compositor. */
    struct Props
    {
        std::string name;                   ///< Connector name (e.g. `DP-1`), the same as its MScreen's.
        std::string description;            ///< Human-readable description.
        std::string make;                   ///< Manufacturer, empty if unknown.
        std::string model;                  ///< Model, empty if unknown.
        std::string serialNumber;           ///< Serial number, empty if unknown.
        SkISize physicalSize {};            ///< Physical size in millimeters, empty if unknown (e.g. projectors).
        std::vector<MOutputMode*> modes;    ///< Modes it supports (may be empty, e.g. virtual outputs).
        bool enabled { false };             ///< Whether it's in use (it has an MScreen then).
        MOutputMode *currentMode {};        ///< While enabled, `nullptr` if it uses a custom mode (or none).
        SkIPoint pos {};                    ///< While enabled, its position in the global space.
        CZTransform transform { CZTransform::Normal }; ///< While enabled, the transform applied to its content.
        Float64 scale { 1.0 };              ///< While enabled, its scale factor.
        bool adaptiveSync { false };        ///< Whether adaptive sync (VRR) is enabled.
    };

    /**
     * @brief How to set an output up, see MOutputManager::apply().
     *
     * config() returns the current one, to change only some fields.
     */
    struct Config
    {
        bool enabled { true };          ///< Disabled outputs ignore the rest.
        MOutputMode *mode {};           ///< One of its modes, or `nullptr` to use customSize (or keep the current one).
        SkISize customSize {};          ///< A custom mode (when mode is `nullptr`), empty to keep the current one.
        Int32 customRefreshRate { 0 };  ///< The custom mode's refresh rate in mHz, 0 for any.
        SkIPoint pos {};                ///< Position in the global space.
        CZTransform transform { CZTransform::Normal }; ///< Transform applied to its content.
        Float64 scale { 1.0 };          ///< Scale factor.
        bool adaptiveSync { false };    ///< Adaptive sync (VRR), set only if it changes (compositors may not support it).
    };

    /** @brief The current properties. */
    const Props &props() const noexcept { return m_current; }

    /** @brief The current configuration (from props()), to pass a modified copy to MOutputManager::apply(). */
    Config config() const noexcept;

    /** @brief Its preferred mode, `nullptr` if none. */
    MOutputMode *preferredMode() const noexcept;

    /** @brief Its MScreen (the wl_output with the same name), `nullptr` while disabled (or not known yet). */
    MScreen *screen() const noexcept;

    /** @brief The zwlr_output_head_v1 proxy. */
    zwlr_output_head_v1 *proxy() const noexcept { return m_proxy; }

    /** @brief User data, destroyed along with this object. */
    std::shared_ptr<CZObject> data;

    /**
     * @brief Emitted when one or more props change (atomically, once the compositor is done
     *        sending them), with the changed ones.
     */
    CZSignal<CZBitset<Changes> /*changes*/> onPropsChanged;

private:
    friend class MOutputManager;
    friend struct MWlrOutputManager;
    friend struct MWlrOutputHead;
    friend struct MWlrOutputMode;
    MOutputHead(zwlr_output_head_v1 *proxy) noexcept : m_proxy(proxy) {}
    ~MOutputHead() noexcept;
    void commit() noexcept; // Applies the pending props (manager done)
    void destroyFinishedModes() noexcept;
    MProxy<zwlr_output_head_v1> m_proxy;
    Props m_current, m_pending;
    CZBitset<Changes> m_changes;
    std::vector<MOutputMode*> m_modes; // Owned: its modes (the finished ones too, until done)
    bool m_isComplete { false };
};

#endif // MOUTPUTHEAD_H
