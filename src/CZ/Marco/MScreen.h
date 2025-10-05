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

class CZ::MScreen : public AKObject
{
public:
    enum Changes
    {
        Position        = 1 << 0,
        PhysicalSize    = 1 << 1,
        PixelGeometry   = 1 << 2,
        Scale           = 1 << 3,
        Name            = 1 << 4,
        Description     = 1 << 5,
        Make            = 1 << 6,
        Model           = 1 << 7,
        Transform       = 1 << 8,
        Modes           = 1 << 9
    };

    struct Mode
    {
        SkISize size;
        Int32 refresh;
        bool isCurrent;
        bool isPreferred;
    };

    struct Props
    {
        std::string name;
        std::string description;
        std::string make;
        std::string model;
        SkIPoint pos;
        SkISize physicalSize;
        Int32 scale { 1 };
        RSubpixel pixelGeometry;
        CZTransform transform;
        std::vector<Mode> modes;
    };

    const Props &props() const noexcept { return m_current; }
    wl_output *wlOutput() const noexcept { return m_proxy.get(); }

    CZSignal<MScreen&, CZBitset<Changes>> onPropsChanged;
private:
    friend class MApp;
    friend class MWlRegistry;
    friend class MWlOutput;
    MScreen(void *proxy, UInt32 name) noexcept : m_proxy(proxy, name) {}
    ~MScreen() { wl_output_destroy(m_proxy); }
    Props m_current, m_pending;
    CZBitset<Changes> m_changes;
    bool m_pendingFirstDone { true };
    MProxy<wl_output> m_proxy;
};

#endif // MSCREEN_H
