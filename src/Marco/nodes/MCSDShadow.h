#ifndef MCSDSHADOW_H
#define MCSDSHADOW_H

#include <Marco/Marco.h>
#include <Marco/MTheme.h>
#include <AK/nodes/AKRenderable.h>
#include <AK/AKWeak.h>

class Marco::MCSDShadow : public AK::AKRenderable
{
public:
    MCSDShadow(MToplevel *toplevel) noexcept;
protected:
    void onRender(const OnRenderParams &params) override;
    AK::AKWeak<MToplevel> m_toplevel;
    sk_sp<SkImage> m_image;
    AK::AKBitset<MTheme::ShadowClamp> m_clampSides;
};

#endif // MCSDSHADOW_H
