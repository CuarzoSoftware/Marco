#ifndef MSUBSURFACE_H
#define MSUBSURFACE_H

#include <Marco/roles/MSurface.h>

class AK::MSubSurface : public MSurface
{
public:
    MSubSurface(MSurface *parent = nullptr) noexcept;
    ~MSubSurface();

    // Can be nullptr
    MSurface *parent() const noexcept;

    // Must not be descendant otherwise noop
    bool setParent(MSurface *surface) noexcept;

    // Must be sibling of the current parent otherwise noop
    // nullptr to place right above the current parent
    bool placeAbove(MSubSurface *subSurface) noexcept;
    // Only sibling
    bool placeBelow(MSubSurface *subSurface) noexcept;

    const SkIPoint &pos() const noexcept;

    // Relative to top left corner of the parent (not the central node)
    void setPos(const SkIPoint &pos) noexcept;
    void setPos(Int32 x, Int32 y) noexcept { setPos({x, y}); };
    class Imp;
    Imp *imp() const noexcept;
private:
    std::unique_ptr<Imp> m_imp;
protected:
    void onUpdate() noexcept override;
    void render() noexcept;
};

#endif // MSUBSURFACE_H
