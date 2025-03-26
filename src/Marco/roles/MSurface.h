#ifndef MWINDOWROLE_H
#define MWINDOWROLE_H

#include <Marco/protocols/viewporter-client.h>
#include <AK/AKSignal.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSolidColor.h>
#include <AK/AKScene.h>
#include <Marco/Marco.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <set>

class AK::MSurface : public AKSolidColor
{
public:
    enum class Role
    {
        Toplevel,
        LayerSurface
    };

    ~MSurface();

    Role role() noexcept;

    /**
     * @brief The current scale factor.
     *
     * @return
     */
    Int32 scale() noexcept;
    const SkISize &surfaceSize() const noexcept;
    const SkISize &bufferSize() const noexcept;
    const std::set<MScreen*> &screens() const noexcept;
    void setMapped(bool mapped) noexcept;
    bool mapped() const noexcept;
    void update() noexcept;
    SkISize minContentSize() noexcept;

    AKScene &scene() const noexcept;
    AKTarget *target() const noexcept;
    AKNode *rootNode() const noexcept;

    wl_surface *wlSurface() const noexcept;
    wl_callback *wlCallback() const noexcept;
    wp_viewport *wlViewport() const noexcept;
    wl_egl_window *eglWindow() const noexcept;
    sk_sp<SkSurface> skSurface() const noexcept;
    EGLSurface eglSurface() const noexcept;

    AKSignal<MScreen&> onMappedChanged;
    AKSignal<MScreen&> onEnteredScreen;
    AKSignal<MScreen&> onLeftScreen;
    AKSignal<UInt32> onCallbackDone;
protected:
    friend class MApplication;
    friend class MCSDShadow;
    MSurface(Role role) noexcept;
    virtual void onUpdate() noexcept;
    class Imp;
    Imp *imp() const noexcept;
private:
    std::unique_ptr<Imp> m_imp;
    using AKNode::setParent;
    using AKNode::parent;
    using AKNode::setVisible;
};

#endif // MWINDOWROLE_H
