#ifndef MOUTPUTMODE_H
#define MOUTPUTMODE_H

#include <CZ/Marco/Protocols/WlrOutputManagement/wlr-output-management-unstable-v1-client.h>
#include <CZ/Marco/Marco.h>
#include <CZ/Marco/MProxy.h>
#include <CZ/AK/AKObject.h>
#include <CZ/skia/core/SkSize.h>

/**
 * @brief A display mode an output supports (wlr-output-management), see MOutputHead::Props::modes.
 *
 * Created and destroyed by its head (MOutputHead). Its props don't change: when the compositor
 * removes a mode, it's taken out of its head's modes (MOutputHead::CHModes) and destroyed.
 */
class CZ::MOutputMode : public AKObject
{
public:
    /** @brief Size in physical pixels (untransformed). */
    SkISize size() const noexcept { return m_size; }

    /** @brief Refresh rate in mHz, 0 if unknown. */
    Int32 refreshRate() const noexcept { return m_refreshRate; }

    /** @brief Whether it's the output's preferred (native) mode. */
    bool preferred() const noexcept { return m_preferred; }

    /** @brief The head (output) it belongs to. */
    MOutputHead &head() const noexcept { return m_head; }

    /** @brief The zwlr_output_mode_v1 proxy. */
    zwlr_output_mode_v1 *proxy() const noexcept { return m_proxy; }

private:
    friend class MOutputHead;
    friend class MOutputManager;
    friend struct MWlrOutputHead;
    friend struct MWlrOutputMode;
    MOutputMode(MOutputHead &head, zwlr_output_mode_v1 *proxy) noexcept : m_head(head), m_proxy(proxy) {}
    ~MOutputMode() noexcept;
    MOutputHead &m_head;
    MProxy<zwlr_output_mode_v1> m_proxy;
    SkISize m_size {};
    Int32 m_refreshRate { 0 };
    bool m_preferred { false };
    bool m_finished { false }; // Removed by the compositor (destroyed once its head is done)
};

#endif // MOUTPUTMODE_H
