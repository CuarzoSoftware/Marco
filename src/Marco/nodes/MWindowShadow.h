#ifndef MWINDOWSHADOW_H
#define MWINDOWSHADOW_H

#include <Marco/Marco.h>
#include <AK/nodes/AKBakeable.h>
#include <AK/AKWeak.h>

#include <include/effects/SkImageFilters.h>

class Marco::MWindowShadow : public AK::AKBakeable
{
public:
    MWindowShadow(MToplevel *toplevel) noexcept;

protected:
    void onBake(OnBakeParams *params) override;
    void onRender(AK::AKPainter *painter, const SkRegion &damage) override;
    SkISize m_size;
    AK::AKWeak<MToplevel> m_toplevel;
    SkPath m_path;
    sk_sp<SkImageFilter> m_filter;
};

#endif // MWINDOWSHADOW_H
