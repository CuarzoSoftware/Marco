#include <CZ/Marco/Extensions/MOutputHead.h>
#include <CZ/Marco/Extensions/MOutputMode.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>

using namespace CZ;

MOutputMode::~MOutputMode() noexcept
{
    if (zwlr_output_mode_v1_get_version(m_proxy) >= ZWLR_OUTPUT_MODE_V1_RELEASE_SINCE_VERSION)
        zwlr_output_mode_v1_release(m_proxy);
    else
        zwlr_output_mode_v1_destroy(m_proxy);
}

MOutputHead::~MOutputHead() noexcept
{
    for (auto *mode : m_modes)
        delete mode;

    if (zwlr_output_head_v1_get_version(m_proxy) >= ZWLR_OUTPUT_HEAD_V1_RELEASE_SINCE_VERSION)
        zwlr_output_head_v1_release(m_proxy);
    else
        zwlr_output_head_v1_destroy(m_proxy);
}

MOutputHead::Config MOutputHead::config() const noexcept
{
    Config config {};
    config.enabled = m_current.enabled;
    config.mode = m_current.currentMode;
    config.pos = m_current.pos;
    config.transform = m_current.transform;
    config.scale = m_current.scale;
    config.adaptiveSync = m_current.adaptiveSync;
    return config;
}

MOutputMode *MOutputHead::preferredMode() const noexcept
{
    for (auto *mode : m_current.modes)
        if (mode->preferred())
            return mode;

    return nullptr;
}

MScreen *MOutputHead::screen() const noexcept
{
    if (!m_current.enabled)
        return nullptr;

    for (auto *screen : MApp::Get()->screens())
        if (screen->props().name == m_current.name)
            return screen;

    return nullptr;
}

void MOutputHead::commit() noexcept
{
    if (!m_isComplete)
    {
        m_isComplete = true;
        m_current = m_pending;
        m_changes.set(0);
        return;
    }

    if (!m_changes)
        return;

    CZBitset<Changes> real {};

    const auto apply = [&](Changes change, auto &current, const auto &pending) {
        if (m_changes.has(change) && current != pending)
        {
            current = pending;
            real.add(change);
        }
    };

    apply(CHName, m_current.name, m_pending.name);
    apply(CHDescription, m_current.description, m_pending.description);
    apply(CHMake, m_current.make, m_pending.make);
    apply(CHModel, m_current.model, m_pending.model);
    apply(CHSerialNumber, m_current.serialNumber, m_pending.serialNumber);
    apply(CHPhysicalSize, m_current.physicalSize, m_pending.physicalSize);
    apply(CHModes, m_current.modes, m_pending.modes);
    apply(CHEnabled, m_current.enabled, m_pending.enabled);
    apply(CHCurrentMode, m_current.currentMode, m_pending.currentMode);
    apply(CHPos, m_current.pos, m_pending.pos);
    apply(CHTransform, m_current.transform, m_pending.transform);
    apply(CHScale, m_current.scale, m_pending.scale);
    apply(CHAdaptiveSync, m_current.adaptiveSync, m_pending.adaptiveSync);
    m_changes.set(0);

    if (real)
    {
        MLog(CZTrace, "Output head {} changed: {}", m_current.name, real.get());
        onPropsChanged.notify(real);
    }
}

void MOutputHead::destroyFinishedModes() noexcept
{
    std::erase_if(m_modes, [](MOutputMode *mode) {
        if (!mode->m_finished)
            return false;

        delete mode;
        return true;
    });
}
