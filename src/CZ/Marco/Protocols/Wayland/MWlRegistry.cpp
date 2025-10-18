#include <CZ/Marco/Protocols/Wayland/MWlRegistry.h>
#include <CZ/Marco/Protocols/Wayland/MWlOutput.h>
#include <CZ/Marco/Protocols/Wayland/MWlSeat.h>
#include <CZ/Marco/Protocols/Viewporter/viewporter-client.h>
#include <CZ/Marco/Protocols/XdgShell/MXdgWmBase.h>
#include <CZ/Marco/Protocols/XdgDecoration/xdg-decoration-unstable-v1-client.h>
#include <CZ/Marco/Protocols/LvrBackgroundBlur/MLvrBackgroundManager.h>
#include <CZ/Marco/Protocols/LvrInvisibleRegion/lvr-invisible-region-client.h>
#include <CZ/Marco/Protocols/LvrSvgPath/lvr-svg-path-client.h>
#include <CZ/Marco/Protocols/WlrLayerShell/wlr-layer-shell-unstable-v1-client.h>
#include <CZ/Marco/Protocols/CursorShape/cursor-shape-v1-client.h>
#include <CZ/Marco/Protocols/WlrForeignToplevel/MWlrForeignToplevelManager.h>
#include <CZ/Marco/MApp.h>

using namespace CZ;

void MWlRegistry::global(void *data, wl_registry *registry, UInt32 name, const char *interface, UInt32 version)
{
    auto &wl { *static_cast<MApp::Wayland*>(data) };

    if (!wl.shm && strcmp(interface, wl_shm_interface.name) == 0)
    {
        wl.shm.set(wl_registry_bind(registry, name, &wl_shm_interface, version), name);
    }
    else if (!wl.compositor && strcmp(interface, wl_compositor_interface.name) == 0)
    {
        wl.compositor.set(wl_registry_bind(registry, name, &wl_compositor_interface, version), name);
    }
    else if (!wl.subCompositor && strcmp(interface, wl_subcompositor_interface.name) == 0)
    {
        wl.subCompositor.set(wl_registry_bind(registry, name, &wl_subcompositor_interface, version), name);
    }
    else if (!wl.xdgWmBase && strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        wl.xdgWmBase.set(wl_registry_bind(registry, name, &xdg_wm_base_interface, version), name);
        xdg_wm_base_add_listener(wl.xdgWmBase, &MXdgWmBase::Listener, NULL);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        wl_output *output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, version);
        MScreen *screen { new MScreen(output, name) };
        wl_output_set_user_data(output, screen);
        wl_output_add_listener(output, &MWlOutput::Listener, screen);
        MApp::Get()->m_pendingScreens.push_back(screen);
    }
    else if (!wl.seat && strcmp(interface, wl_seat_interface.name) == 0)
    {
        wl.seat.set(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 9u)), name);
        wl_seat_add_listener(wl.seat, &MWlSeat::Listener, NULL);
    }
    else if (!wl.xdgDecorationManager && strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
    {
        wl.xdgDecorationManager.set(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, version), name);
    }
    else if (!wl.viewporter && strcmp(interface, wp_viewporter_interface.name) == 0)
    {
        wl.viewporter.set(wl_registry_bind(registry, name, &wp_viewporter_interface, version), name);
    }
    else if (!wl.layerShell && strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
    {
        wl.layerShell.set(wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version), name);
    }
    else if (!wl.backgroundBlurManager && strcmp(interface, lvr_background_blur_manager_interface.name) == 0)
    {
        wl.backgroundBlurManager.set(wl_registry_bind(registry, name, &lvr_background_blur_manager_interface, version), name);
        lvr_background_blur_manager_add_listener(wl.backgroundBlurManager, &MLvrBackgroundBlurManager::Listener, NULL);
    }
    else if (!wl.svgPathManager && strcmp(interface, lvr_svg_path_manager_interface.name) == 0)
    {
        wl.svgPathManager.set(wl_registry_bind(registry, name, &lvr_svg_path_manager_interface, version), name);
    }
    else if (!wl.invisibleRegionManager && strcmp(interface, lvr_invisible_region_manager_interface.name) == 0)
    {
        wl.invisibleRegionManager.set(wl_registry_bind(registry, name, &lvr_invisible_region_manager_interface, version), name);
    }
    else if (!wl.cursorShapeManager && strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0)
    {
        wl.cursorShapeManager.set(wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, version), name);
    }
    else if (!wl.foreignToplevelManager && strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
    {
        wl.foreignToplevelManager.set(wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, version), name);
        zwlr_foreign_toplevel_manager_v1_add_listener(wl.foreignToplevelManager, &MWlrForeignToplevelManager::Listener, nullptr);
        MWlrForeignToplevelManager::bound();
    }
}

void MWlRegistry::global_remove(void *data, wl_registry *registry, UInt32 name)
{
    CZ_UNUSED(data)
    CZ_UNUSED(registry)

    auto app { MApp::Get() };

    for (size_t i = 0; i < app->m_screens.size(); i++)
    {
        if (app->m_screens[i]->m_proxy.name() == name)
        {
            if (app->m_running)
                app->onScreenUnplugged.notify(*app->m_screens[i]);

            delete app->m_screens[i];
            app->m_screens[i] = app->m_screens.back();
            app->m_screens.pop_back();
            break;
        }
    }

    for (size_t i = 0; i < app->m_pendingScreens.size(); i++)
    {
        if (app->m_pendingScreens[i]->m_proxy.name() == name)
        {
            delete app->m_pendingScreens[i];
            app->m_pendingScreens[i] = app->m_pendingScreens.back();
            app->m_pendingScreens.pop_back();
            break;
        }
    }
}
