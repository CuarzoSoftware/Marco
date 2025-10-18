#ifndef MFOREIGNTOPLEVELMANAGER_H
#define MFOREIGNTOPLEVELMANAGER_H

#include <CZ/AK/AKObject.h>
#include <CZ/Marco/Marco.h>
#include <unordered_set>

class CZ::MForeignToplevelManager : public AKObject
{
public:
    const std::unordered_set<MForeignToplevel*> &toplevels() const noexcept { return m_toplevels; }

    CZSignal<MForeignToplevel*> onAdded;
    CZSignal<MForeignToplevel*> onRemoved;
private:
    friend class MApp;
    friend class MWlrForeignToplevelManager;
    friend class MWlrForeignToplevelHandle;
    MForeignToplevelManager() noexcept = default;
    void init() noexcept;
    void unit() noexcept;
    std::unordered_set<MForeignToplevel*> m_toplevels;
    std::unordered_set<MForeignToplevel*> m_incomplete;
    CZListener *m_screenListener {};
};

#endif // MFOREIGNTOPLEVELMANAGER_H
