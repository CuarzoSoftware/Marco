#ifndef MFOREIGNTOPLEVELMANAGER_H
#define MFOREIGNTOPLEVELMANAGER_H

#include <CZ/AK/AKObject.h>
#include <CZ/Marco/Marco.h>
#include <unordered_set>

/**
 * @brief Lists the windows of other clients (wlr-foreign-toplevel-management).
 *
 * Accessed through MApp::ext. Only works if MForeignToplevelManagerExt was passed to MApp::Make()
 * and the compositor supports the protocol; otherwise the list stays empty.
 *
 * @code
 * MOptions options;
 * options.extensions.add(MForeignToplevelManagerExt);
 * auto app { MApp::Make(options) };
 * app->ext.foreignToplevelManager.onAdded.subscribe(this, [](MForeignToplevel *toplevel) {
 *     // ...
 * });
 * @endcode
 */
class CZ::MForeignToplevelManager : public AKObject
{
public:
    /** @brief The current windows (only those whose initial props are known). */
    const std::unordered_set<MForeignToplevel*> &toplevels() const noexcept { return m_toplevels; }

    /** @brief Emitted when a window is added to toplevels(). */
    CZSignal<MForeignToplevel*> onAdded;

    /** @brief Emitted when a window is closed, right before it is destroyed. */
    CZSignal<MForeignToplevel*> onRemoved;
private:
    friend class MApp;
    friend struct MWlrForeignToplevelManager;
    friend struct MWlrForeignToplevelHandle;
    MForeignToplevelManager() noexcept = default;
    void init() noexcept;
    void unit() noexcept;
    std::unordered_set<MForeignToplevel*> m_toplevels;
    std::unordered_set<MForeignToplevel*> m_incomplete;
    CZListener *m_screenListener {};
};

#endif // MFOREIGNTOPLEVELMANAGER_H
