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

class Marco::MSurface : public AK::AKSolidColor
{
public:
    enum class Role
    {
        Toplevel
    };

    Role role() noexcept
    {
        return m_role;
    }

    Int32 scale() noexcept
    {
        return m_scale;
    }

    const SkISize &surfaceSize() const noexcept
    {
        return m_surfaceSize;
    }

    const SkISize &bufferSize() const noexcept
    {
        return m_bufferSize;
    }

    void show() noexcept
    {
        if (m_visible)
            return;

        m_visible = true;
        m_changes.set(CHVisibility);
        updateLater();
    }

    void hide() noexcept
    {
        if (!m_visible)
            return;

        m_visible = false;
        m_changes.set(CHVisibility);
        updateLater();
    }

    void updateLater() noexcept
    {
        m_changes.set(CHUpdateLater);
    }

    ~MSurface();

    struct
    {
        AK::AKSignal<MScreen&> enteredScreen;
        AK::AKSignal<MScreen&> leftScreen;
    } on;
protected:
    friend class MApplication;

    enum Changes
    {
        CHUpdateLater,
        CHScale,
        CHPreferredBufferScale,
        CHSize,
        CHVisibility,
        CHLast
    };

    std::bitset<128> m_changes;

    MSurface(Role role) noexcept;
    virtual void handleChanges() noexcept = 0;

    AK::AKScene m_scene;
    AK::AKWeak<AK::AKTarget> m_target;
    AK::AKContainer m_root;
    wl_surface *m_wlSurface { nullptr };
    wl_egl_window *m_eglWindow { nullptr };
    EGLSurface m_eglSurface { EGL_NO_SURFACE };
    sk_sp<SkSurface> m_skSurface;
    SkISize m_surfaceSize { 0, 0 };
    SkISize m_bufferSize { 0, 0 };

    Int32 m_preferredBufferScale { -1 };
    Int32 m_scale { 1 };

    bool m_visible { false };    
private:
    Role m_role;
    size_t m_appLink;
};

#endif // MWINDOWROLE_H
