#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Private/MPopupPrivate.h>
#include <CZ/Marco/Private/MToplevelPrivate.h>
#include <CZ/Marco/Private/MLayerSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Ream/WL/RWLSwapchain.h>
#include <CZ/Core/CZCore.h>
#include <CZ/AK/Events/AKVibrancyEvent.h>

using namespace CZ;

static wl_surface_listener wlSurfaceListener;
static wl_callback_listener wlCallbackListener;
static lvr_background_blur_listener backgroundBlurListener;

MSurface::Imp::Imp(MSurface &obj) noexcept : obj(obj), root(obj)
{
    wlSurfaceListener.enter = wl_surface_enter;
    wlSurfaceListener.leave = wl_surface_leave;
    wlSurfaceListener.preferred_buffer_scale = wl_surface_preferred_buffer_scale;
    wlSurfaceListener.preferred_buffer_transform = wl_surface_preferred_buffer_transform;
    wlCallbackListener.done = wl_callback_done;
    backgroundBlurListener.state = background_blur_state;
    backgroundBlurListener.configure = background_blur_configure;
}

void MSurface::Imp::wl_surface_enter(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };

    if (surface.imp()->screens.contains(screen))
        return;

    surface.imp()->screens.insert(screen);
    surface.imp()->tmpFlags.add(Imp::ScreensChanged);
    surface.onEnteredScreen.notify(*screen);

    if (surface.imp()->preferredBufferScale == -1)
        surface.update();

    // TODO: Listen for screen prop changes
}

void MSurface::Imp::wl_surface_leave(void *data, wl_surface */*surface*/, wl_output *output)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    MScreen *screen { static_cast<MScreen*>(wl_output_get_user_data(output)) };
    auto it = surface.imp()->screens.find(screen);

    if (it == surface.imp()->screens.end())
        return;

    surface.imp()->screens.erase(it);
    surface.imp()->tmpFlags.add(Imp::ScreensChanged);
    surface.onLeftScreen.notify(*screen);

    if (surface.imp()->preferredBufferScale == -1)
        surface.update();
}

void MSurface::Imp::wl_surface_preferred_buffer_scale(void *data, wl_surface */*surface*/, Int32 factor)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    if (factor <= 0)
        factor = 1;

    if (surface.imp()->preferredBufferScale == factor)
        return;

    surface.imp()->preferredBufferScale = factor;
    surface.imp()->tmpFlags.add(Imp::PreferredScaleChanged);
    surface.update();
}

void MSurface::Imp::wl_surface_preferred_buffer_transform(void */*data*/, wl_surface */*surface*/, UInt32 /*transform*/)
{
    // TODO
}

void MSurface::Imp::wl_callback_done(void *data, wl_callback *callback, UInt32 ms)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    wl_callback_destroy(callback);
    surface.imp()->wlCallback = nullptr;
    surface.onCallbackDone.notify(ms);

    if (surface.imp()->flags.has(PendingUpdate) || surface.target()->isDirty())
        surface.update();
}

void MSurface::Imp::background_blur_state(void *data, lvr_background_blur *, UInt32 state)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    surface.imp()->pendingVibrancyState = (AKVibrancyState)state;
}

void MSurface::Imp::background_blur_configure(void *data, lvr_background_blur *backgroundBlur, UInt32 serial)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    lvr_background_blur_ack_configure(backgroundBlur, serial);
    auto core { CZCore::Get() };

    if (surface.imp()->pendingVibrancyState != surface.imp()->currentVibrancyState)
    {
        surface.update(true);
        surface.imp()->currentVibrancyState = surface.imp()->pendingVibrancyState;
        core->sendEvent(AKVibrancyEvent(
            surface.imp()->currentVibrancyState),
            surface);
    }
}

void MSurface::Imp::createSurface() noexcept
{
    auto app { MApp::Get() };

    if (wlCallback)
    {
        wl_callback_destroy(wlCallback);
        wlCallback = nullptr;
    }

    if (wlViewport)
    {
        wp_viewport_destroy(wlViewport);
        wlViewport = nullptr;
    }

    swapchain.reset();

    if (backgroundBlur)
    {
        lvr_background_blur_destroy(backgroundBlur);
        backgroundBlur = nullptr;
    }

    if (invisibleRegion)
    {
        lvr_invisible_region_destroy(invisibleRegion);
        invisibleRegion = nullptr;
    }

    if (wlSurface)
    {
        wl_surface_destroy(wlSurface);
        wlSurface = nullptr;
    }

    wlSurface = wl_compositor_create_surface(app->wl.compositor);
    wl_surface_add_listener(wlSurface, &wlSurfaceListener, &obj);
    wlViewport = wp_viewporter_get_viewport(app->wl.viewporter, wlSurface);

    if (app->wl.invisibleRegionManager)
        invisibleRegion = lvr_invisible_region_manager_get_invisible_region(app->wl.invisibleRegionManager, wlSurface);

    if (app->wl.backgroundBlurManager)
    {
        backgroundBlur = lvr_background_blur_manager_get_background_blur(app->wl.backgroundBlurManager, wlSurface);
        lvr_background_blur_add_listener(backgroundBlur, &backgroundBlurListener, &obj);
    }
}

bool MSurface::Imp::createCallback() noexcept
{
    if (wlCallback)
        return false;

    callbackSendMs = CZTime::Ms();
    wlCallback = wl_surface_frame(wlSurface);
    wl_callback_add_listener(wlCallback, &wlCallbackListener, &obj);
    return true;
}

void MSurface::Imp::setMapped(bool mapped) noexcept
{
    if (mapped == obj.mapped())
        return;

    if (!mapped && wlCallback)
    {
        wl_callback_destroy(wlCallback);
        wlCallback = nullptr;
    }

    std::unordered_set<MPopup*> *childPopups { nullptr };

    switch (role)
    {
    case Role::Popup:
        childPopups = &((MPopup*)&obj)->imp()->childPopups;
        break;
    case Role::Toplevel:
        childPopups = &((MToplevel*)&obj)->imp()->childPopups;
        break;
    case Role::LayerSurface:
        childPopups = &((MLayerSurface*)&obj)->imp()->childPopups;
        break;
    default:
        break;
    }

    if (childPopups)
        for (auto &child : *childPopups)
            child->update();

    flags.setFlag(Mapped, mapped);
    obj.onMappedChanged.notify();
}

bool MSurface::Imp::resizeBuffer(const SkISize &size) noexcept
{
    const SkISize bufferSize { size.width() * scale , size.height() * scale };

    if (bufferSize == this->bufferSize)
        return false;

    this->size = size;
    this->bufferSize = bufferSize;

    if (size.isEmpty())
    {
        swapchain.reset();
        return true;
    }

    if (swapchain)
    {
        if (swapchain->size().width() < bufferSize.width() || swapchain->size().height() < bufferSize.height())
        {
            swapchain->resize(bufferSize);
            return true;
        }

        return false;
    }
    else
    {
        swapchain = RWLSwapchain::Make(wlSurface, bufferSize);
        assert("Failed to create EGLSurface for MSurface" && swapchain);
    }

    return true;
}
