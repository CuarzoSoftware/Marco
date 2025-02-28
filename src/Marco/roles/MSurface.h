#ifndef MWINDOWROLE_H
#define MWINDOWROLE_H

#include <AK/AKSignal.h>
#include <AK/nodes/AKContainer.h>
#include <AK/nodes/AKSolidColor.h>
#include <AK/AKScene.h>
#include <Marco/Marco.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <set>

class Marco::MSurface : public AK::AKSolidColor
{
public:
    enum class Role
    {
        Toplevel
    };

    ~MSurface();

    Role role() noexcept
    {
        return m_role;
    }

    Int32 scale() noexcept
    {
        return cl.scale;
    }

    const SkISize &surfaceSize() const noexcept
    {
        return cl.size;
    }

    const SkISize &bufferSize() const noexcept
    {
        return cl.bufferSize;
    }

    const std::set<MScreen*> &screens() const noexcept
    {
        return se.screens;
    }

    void show() noexcept
    {
        setVisible(true);
    }

    void hide() noexcept
    {
        setVisible(false);
    }

    void update() noexcept;
    SkISize minContentSize() noexcept;

    wl_surface *wlSurface() const noexcept { return wl.surface; }

    struct
    {
        AK::AKSignal<MScreen&> enteredScreen;
        AK::AKSignal<MScreen&> leftScreen;
        AK::AKSignal<UInt32> presented; // ms
    } on;
protected:
    friend class MApplication;
    friend class MCSDShadow;
    MSurface(Role role) noexcept;
    virtual void onUpdate() noexcept;

    bool createCallback() noexcept;

    /**
     * @brief Resize the wl_egl_window
     *
     * @return true if resized, false if the size is the same.
     */
    bool resizeBuffer(const SkISize &size) noexcept;

    enum ClientChanges
    {
        Cl_Scale,
        Cl_Last
    };

    enum ServerChanges
    {
        Se_Screens,
        Se_PrefferredScale,
        Se_Last
    };

    struct {
        AK::AKScene scene;
        AK::AKWeak<AK::AKTarget> target;
        AK::AKContainer root;
    } ak;

    struct {
        UInt32 callbackSendMs { 0 };
        wl_callback *callback { nullptr };
        wl_surface *surface { nullptr };
    } wl;

    struct {
        wl_egl_window *eglWindow { nullptr };
        EGLSurface eglSurface { EGL_NO_SURFACE };
        sk_sp<SkSurface> skSurface;
    } gl;

    struct {
        std::bitset<128> changes;
        SkISize size { 0, 0 };
        SkISize bufferSize { 0, 0 };
        Int32 scale { 1 };
        bool pendingUpdate { true };
    } cl;

    struct {
        std::bitset<128> changes;
        std::set<MScreen*>screens;
        Int32 preferredBufferScale { -1 };
    } se;

private:
    using AKNode::setParent;
    using AKNode::parent;
    using AKNode::setVisible;
    static void wl_surface_enter(void *data, wl_surface *surface, wl_output *output);
    static void wl_surface_leave(void *data,wl_surface *surface, wl_output *output);
    static void wl_surface_preferred_buffer_scale(void *data, wl_surface *surface, Int32 factor);
    static void wl_surface_preferred_buffer_transform(void *data, wl_surface *surface, UInt32 transform);
    static void wl_callback_done(void *data, wl_callback *callback, UInt32 ms);
    Role m_role;
    size_t m_appLink;
};

#endif // MWINDOWROLE_H
