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
    void onSceneCalculatedRect() override;
    void onRender(AK::AKPainter *painter, const SkRegion &damage) override;
    AK::AKWeak<MToplevel> m_toplevel;
    sk_sp<SkImage> m_image;
    SkISize m_prevSize { 0, 0 };
    Int32 m_prevScale { 1 };
    AK::AKBitset<MTheme::ShadowClamp> m_clampSides;
};

#endif // MCSDSHADOW_H
