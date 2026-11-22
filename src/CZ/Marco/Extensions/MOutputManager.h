#ifndef MOUTPUTMANAGER_H
#define MOUTPUTMANAGER_H

#include <CZ/Marco/Extensions/MOutputHead.h>
#include <CZ/Marco/Extensions/MOutputMode.h>
#include <CZ/AK/AKObject.h>
#include <CZ/Marco/Marco.h>
#include <functional>
#include <unordered_map>
#include <vector>

/**
 * @brief Lists the outputs (displays) and changes how they're set up (wlr-output-management).
 *
 * Accessed through MApp::ext. Only works if MOutputManagerExt was passed to MApp::Make() and the
 * compositor supports the protocol (available() is `false` otherwise, and heads() stays empty).
 *
 * Each output is an MOutputHead, enabled or not, with the modes it supports (MOutputMode). Changes
 * the compositor sends are applied atomically: each new head is announced with onAdded, changed
 * ones emit MOutputHead::onPropsChanged, and then onChanged is emitted. The initial state usually
 * arrives during MApp::Make() (before anything can subscribe), so read heads() right after it.
 *
 * To change the setup, pass the new configuration of the heads that change to apply() (the others
 * keep theirs). The compositor applies all of it or nothing:
 *
 * @code
 * MOptions options;
 * options.extensions.add(MOutputManagerExt);
 * auto app { MApp::Make(options) };
 *
 * // The heads are known by now (later changes: onChanged)
 * auto &manager { app->ext.outputManager };
 * MOutputHead *head { manager.heads().front() };
 * MOutputHead::Config config { head->config() };
 * config.scale = 2.0;
 * manager.apply({ { head, config } }, [](MOutputManager::Result result) {
 *     // ...
 * });
 * @endcode
 */
class CZ::MOutputManager : public AKObject
{
public:
    /** @brief How an apply() or test() ended. */
    enum class Result
    {
        Succeeded, ///< Applied (or, for test(), it would work).
        Failed,    ///< Rejected by the compositor (nothing changed).
        Cancelled  ///< The outputs changed meanwhile (nothing changed): try again with the new state.
    };

    /** @brief Whether the compositor supports the protocol (and MOutputManagerExt was enabled). */
    bool available() const noexcept;

    /** @brief The outputs, in the order the compositor announced them. */
    const std::vector<MOutputHead*> &heads() const noexcept { return m_heads; }

    /** @brief The head named @p name (e.g. `DP-1`), `nullptr` if none. */
    MOutputHead *find(const std::string &name) const noexcept;

    /**
     * @brief Asks the compositor to set the outputs up as @p configs says.
     *
     * Heads not in @p configs keep their current configuration (MOutputHead::config()). @p done is
     * called once with the result, unless the app quits first. Like the props, the new state
     * arrives later through MOutputHead::onPropsChanged and onChanged.
     *
     * @return `false` (and @p done isn't called) if not available() or no heads are known yet.
     */
    bool apply(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
               std::function<void(Result)> done = {}) noexcept;

    /** @brief Like apply(), but only checks whether the compositor would accept it (nothing changes). */
    bool test(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
              std::function<void(Result)> done) noexcept;

    /** @brief Emitted when a head is added to heads(). */
    CZSignal<MOutputHead*> onAdded;

    /** @brief Emitted when a head is removed (e.g. the display was unplugged), right before it's destroyed. */
    CZSignal<MOutputHead*> onRemoved;

    /** @brief Emitted after the compositor sent a consistent state (after that batch's onAdded and MOutputHead::onPropsChanged). */
    CZSignal<> onChanged;

private:
    friend class MApp;
    friend struct MWlrOutputManager;
    friend struct MWlrOutputHead;
    friend struct MWlrOutputConfiguration;
    MOutputManager() noexcept = default;
    bool configure(const std::unordered_map<MOutputHead*, MOutputHead::Config> &configs,
                   std::function<void(Result)> done, bool test) noexcept;
    void removeHead(MOutputHead *head) noexcept;
    void unit() noexcept;

    struct Configuration
    {
        zwlr_output_configuration_v1 *proxy;
        std::vector<zwlr_output_configuration_head_v1*> heads;
        std::function<void(Result)> done;
    };
    void finish(Configuration *configuration, Result result) noexcept;

    std::vector<MOutputHead*> m_heads;
    std::vector<MOutputHead*> m_incomplete; // Waiting for the first done
    std::vector<Configuration*> m_configurations; // Waiting for the result
    UInt32 m_serial { 0 };
    bool m_hasSerial { false };
};

#endif // MOUTPUTMANAGER_H
