#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/Private/MPopupPrivate.h>
#include <CZ/Marco/Private/MToplevelPrivate.h>
#include <CZ/Marco/Private/MLayerSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
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

static UInt32 ToLvrColorScheme(CZColorScheme scheme) noexcept
{
    switch (scheme)
    {
    case CZColorScheme::Dark:  return LVR_BACKGROUND_BLUR_COLOR_SCHEME_DARK;
    case CZColorScheme::Light: return LVR_BACKGROUND_BLUR_COLOR_SCHEME_LIGHT;
    default:                   return LVR_BACKGROUND_BLUR_COLOR_SCHEME_UNKNOWN;
    }
}

bool MSurface::Imp::updateBlurMaterial(bool scheduleCommit) noexcept
{
    if (!backgroundBlur || wl_proxy_get_version((wl_proxy*)backgroundBlur) < 2)
        return false;

    std::optional<std::tuple<wl_fixed_t, wl_fixed_t, UInt32>> material;

    if (blurMaterial)
        material = std::make_tuple(
            wl_fixed_from_double(std::max(0.f, blurMaterial->radius)),
            wl_fixed_from_double(std::max(0.f, blurMaterial->saturation)),
            UInt32(blurMaterial->tint.resolve(obj.colorScheme())));

    if (material == sentBlurMaterial)
        return false;

    sentBlurMaterial = material;

    if (material)
        lvr_background_blur_set_material(backgroundBlur, std::get<0>(*material), std::get<1>(*material), std::get<2>(*material));
    else
        lvr_background_blur_unset_material(backgroundBlur);

    if (scheduleCommit)
        obj.update(true); // double-buffered request → schedule a commit

    return true;
}

bool MSurface::Imp::updateBlurOpacity(bool scheduleCommit) noexcept
{
    if (!backgroundBlur || wl_proxy_get_version((wl_proxy*)backgroundBlur) < 2)
        return false;

    const wl_fixed_t opacity { wl_fixed_from_double(std::clamp(blurOpacity * obj.opacity(), 0.f, 1.f)) };

    if (opacity == sentBlurOpacity)
        return false;

    sentBlurOpacity = opacity;
    lvr_background_blur_set_opacity(backgroundBlur, opacity);

    if (scheduleCommit)
        obj.update(true); // double-buffered request → schedule a commit

    return true;
}

void MSurface::Imp::updateBlurColorScheme() noexcept
{
    if (!backgroundBlur)
        return;

    updateBlurMaterial(true); // The tint follows the color scheme

    const CZColorScheme scheme { obj.colorScheme() };

    if (scheme == sentBlurScheme)
        return;

    sentBlurScheme = scheme;
    lvr_background_blur_set_color_scheme(backgroundBlur, ToLvrColorScheme(scheme));
    obj.update(true); // double-buffered request → schedule a commit
}

void MSurface::Imp::background_blur_configure(void *data, lvr_background_blur *backgroundBlur, UInt32 serial)
{
    MSurface &surface { *static_cast<MSurface*>(data) };
    lvr_background_blur_ack_configure(backgroundBlur, serial);

    // The color scheme may be set once a configure has been acknowledged.
    surface.imp()->updateBlurColorScheme();

    surface.imp()->currentVibrancyState = surface.imp()->pendingVibrancyState;
    surface.imp()->updateVibrancyState();
}

void MSurface::Imp::updateVibrancyState() noexcept
{
    AKVibrancyState state { currentVibrancyState };

    if (role == Role::Toplevel && static_cast<MToplevel&>(obj).fullscreen())
        state = AKVibrancyState::Disabled;

    if (state == vibrancyState)
        return;

    vibrancyState = state;
    obj.update(true); // The blur region follows it (see HandleBackgroundBlur())
    CZCore::Get()->sendEvent(AKVibrancyEvent(state), obj);
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
        sentBlurMaskSVG.clear();
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
        sentBlurMaskSVG.clear();
        lvr_background_blur_add_listener(backgroundBlur, &backgroundBlurListener, &obj);
        sentBlurScheme = CZColorScheme::Unknown; // fresh object: its scheme is unknown again
        sentBlurMaterial.reset();                // and its material unset
        sentBlurOpacity = wl_fixed_from_int(1);  // and opaque

        // The compositor enables background blur by default on this object. Suppress it with an
        // empty region so it never shows unless a role opts in (e.g. MToplevel sets a real region
        // when vibrancy is enabled). Without this, roles that never manage the blur (popups,
        // subsurfaces, layer surfaces) would have their whole surface blurred — including the
        // transparent CSD shadow margins, which then read as an opaque backdrop behind the shadow.
        wl_region *empty { wl_compositor_create_region(app->wl.compositor) };
        lvr_background_blur_set_region(backgroundBlur, empty);
        wl_region_destroy(empty);
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

bool MSurface::Imp::resizeBuffer(SkISize size, Int32 step) noexcept
{
    if (size.width() < 0)
    {
        MLog(CZWarning, CZLN, "Invalid width {}. Falling back to 0.", size.width());
        size.fWidth = 0;
    }

    if (size.height() < 0)
    {
        MLog(CZWarning, CZLN, "Invalid height {}. Falling back to 0.", size.height());
        size.fHeight = 0;
    }

    if (step < 1)
    {
        MLog(CZWarning, CZLN, "Invalid step {}. Falling back to 1.", step);
        step = 1;
    }

    const bool sizeChanged = this->size != size;
    this->size = size;

    if (size.isEmpty())
    {
        if (!swapchain)
            return sizeChanged;

        swapchain.reset();
        return true;
    }

    const SkISize bufferSize {
        size.width() * scale,
        size.height() * scale
    };

    const auto roundUp = [](Int32 value, Int32 step) -> Int32
    {
        return (value / step + (value % step != 0)) * step;
    };

    const SkISize allocationSize {
        roundUp(bufferSize.width(), step),
        roundUp(bufferSize.height(), step)
    };

    if (swapchain && swapchain->size() == allocationSize)
        return sizeChanged;

    if (swapchain)
    {
        swapchain->resize(allocationSize);
    }
    else
    {
        swapchain = RWLSwapchain::Make(wlSurface, allocationSize);
        assert("Failed to create EGLSurface for MSurface" && swapchain);
    }

    return true;
}
