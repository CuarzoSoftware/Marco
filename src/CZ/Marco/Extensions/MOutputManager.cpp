#include <CZ/Marco/Protocols/WlrOutputManagement/MWlrOutputManager.h>
#include <CZ/Marco/Extensions/MOutputManager.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <algorithm>

using namespace CZ;

bool MOutputManager::available() const noexcept
{
    auto app { MApp::Get() };
    return app && app->wl.outputManager;
}

MOutputHead *MOutputManager::find(const std::string &name) const noexcept
{
    for (auto *head : m_heads)
        if (head->props().name == name)
            return head;

    return nullptr;
}

bool MOutputManager::apply(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
                           std::function<void(Result)> done) noexcept
{
    return configure(configs, std::move(done), false);
}

bool MOutputManager::test(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
                          std::function<void(Result)> done) noexcept
{
    return configure(configs, std::move(done), true);
}

bool MOutputManager::configure(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
                               std::function<void(Result)> done, bool test) noexcept
{
    if (!available() || !m_hasSerial || m_heads.empty())
        return false;

    auto *configuration { new Configuration() };
    configuration->done = std::move(done);
    configuration->proxy = zwlr_output_manager_v1_create_configuration(MApp::Get()->wl.outputManager, m_serial);
    zwlr_output_configuration_v1_add_listener(configuration->proxy, &MWlrOutputConfiguration::Listener, configuration);

    // Every head must be included: the ones not in configs keep theirs
    for (auto *head : m_heads)
    {
        const auto it { configs.find(head) };
        const MOutputHead::Config config { it == configs.end() ? head->config() : it->second };

        if (!config.enabled)
        {
            zwlr_output_configuration_v1_disable_head(configuration->proxy, head->proxy());
            continue;
        }

        auto *ch { zwlr_output_configuration_v1_enable_head(configuration->proxy, head->proxy()) };
        configuration->heads.emplace_back(ch);

        // One of its modes, a custom one, or the current one (none: the compositor picks one)
        MOutputMode *mode { config.mode };

        if (mode && (&mode->head() != head || mode->m_finished))
        {
            MLog(CZWarning, CZLN, "[MOutputManager::apply] Ignoring a mode of another head (or removed) for {}", head->props().name);
            mode = nullptr;
        }

        if (mode)
            zwlr_output_configuration_head_v1_set_mode(ch, mode->proxy());
        else if (!config.customSize.isEmpty())
            zwlr_output_configuration_head_v1_set_custom_mode(ch, config.customSize.width(), config.customSize.height(), config.customRefreshRate);
        else if (head->props().currentMode)
            zwlr_output_configuration_head_v1_set_mode(ch, head->props().currentMode->proxy());

        zwlr_output_configuration_head_v1_set_position(ch, config.pos.x(), config.pos.y());
        zwlr_output_configuration_head_v1_set_transform(ch, static_cast<Int32>(config.transform));
        zwlr_output_configuration_head_v1_set_scale(ch, wl_fixed_from_double(config.scale));

        // Only if it changes (compositors that don't support it fail otherwise)
        if (config.adaptiveSync != head->props().adaptiveSync &&
            zwlr_output_configuration_head_v1_get_version(ch) >= ZWLR_OUTPUT_CONFIGURATION_HEAD_V1_SET_ADAPTIVE_SYNC_SINCE_VERSION)
            zwlr_output_configuration_head_v1_set_adaptive_sync(ch, config.adaptiveSync ?
                ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED);
    }

    if (test)
        zwlr_output_configuration_v1_test(configuration->proxy);
    else
        zwlr_output_configuration_v1_apply(configuration->proxy);

    m_configurations.emplace_back(configuration);
    return true;
}

void MOutputManager::finish(Configuration *configuration, Result result) noexcept
{
    std::erase(m_configurations, configuration);

    for (auto *ch : configuration->heads)
        zwlr_output_configuration_head_v1_destroy(ch);

    zwlr_output_configuration_v1_destroy(configuration->proxy);
    auto done { std::move(configuration->done) };
    delete configuration;

    if (done)
        done(result);
}

void MOutputManager::removeHead(MOutputHead *head) noexcept
{
    if (std::erase(m_incomplete, head))
    {
        delete head;
        return;
    }

    if (std::erase(m_heads, head))
    {
        MLog(CZTrace, "Output head removed: {}", head->props().name);
        onRemoved.notify(head);
        delete head;
    }
}

void MOutputManager::unit() noexcept
{
    while (!m_configurations.empty())
        finish(m_configurations.back(), Result::Cancelled);

    while (!m_incomplete.empty())
        removeHead(m_incomplete.back());

    while (!m_heads.empty())
        removeHead(m_heads.back());

    m_hasSerial = false;
}
