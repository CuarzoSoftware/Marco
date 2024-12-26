#include <Marco/roles/MSurface.h>
#include <Marco/MApplication.h>
#include <AK/AKColors.h>

using namespace Marco;

static wl_surface_listener wlSurfaceListener;
static wl_callback_listener wlCallbackListener;

MSurface::~MSurface()
{
    app()->m_surfaces[m_appLink] = app()->m_surfaces.back();
    app()->m_surfaces.pop_back();
    m_scene.destroyTarget(m_target);
}

MSurface::MSurface(Role role) noexcept : AK::AKSolidColor(AK::AKColor::GrayLighten5), m_role(role)
{
    wlSurfaceListener.enter = wl_surface_enter;
    wlSurfaceListener.leave = wl_surface_leave;
    wlSurfaceListener.preferred_buffer_scale = wl_surface_preferred_buffer_scale;
    wlSurfaceListener.preferred_buffer_transform = wl_surface_preferred_buffer_transform;
    wlCallbackListener.done = wl_callback_done;

    m_wlSurface = wl_compositor_create_surface(app()->wayland().compositor);
    wl_surface_add_listener(m_wlSurface, &wlSurfaceListener, this);

    m_appLink = app()->m_surfaces.size();
    app()->m_surfaces.push_back(this);
    setParent(&m_root);
    enableChildrenClipping(true);
    m_target.reset(m_scene.createTarget());
    m_target->setRoot(&m_root);
    m_changes.set(CHSize);

    m_target->on.markedDirty.subscribe(this, [this](AK::AKTarget&){
        updateLater();
    });

    app()->on.screenUnplugged.subscribe(this, [this](MScreen &screen){
        wl_surface_leave(this, m_wlSurface, screen.wlOutput());
    });
}

void MSurface::wl_surface_enter(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };

    if (surface.m_screens.contains(screen))
        return;

    surface.m_screens.insert(screen);
    surface.on.enteredScreen.notify(*screen);
}

void MSurface::wl_surface_leave(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };
    auto it = surface.m_screens.find(screen);

    if (it == surface.m_screens.end())
        return;

    surface.m_screens.erase(it);
    surface.on.leftScreen.notify(*screen);
}

void MSurface::wl_surface_preferred_buffer_scale(void *data, wl_surface */*surface*/, Int32 factor)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    surface.m_preferredBufferScale = factor;
    surface.m_changes.set(CHPreferredBufferScale);
    surface.updateLater();
}

void MSurface::wl_surface_preferred_buffer_transform(void */*data*/, wl_surface */*surface*/, UInt32 /*transform*/)
{
    // TODO
}

void MSurface::wl_callback_done(void *data, wl_callback *callback, UInt32 ms)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    wl_callback_destroy(callback);
    surface.m_wlCallback = nullptr;
    surface.on.presented.notify(ms);
}

bool MSurface::createCallback() noexcept
{
    if (m_wlCallback)
        return false;

    m_wlCallback = wl_surface_frame(m_wlSurface);
    wl_callback_add_listener(m_wlCallback, &wlCallbackListener, this);
    return true;
}
