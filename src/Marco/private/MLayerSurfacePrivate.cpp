#include <Marco/private/MLayerSurfacePrivate.h>

using namespace AK;

void MLayerSurface::Imp::configure(void *data, zwlr_layer_surface_v1 *layerSurface, UInt32 serial, UInt32 width, UInt32 height)
{
    auto &role { *static_cast<MLayerSurface*>(data) };
    role.imp()->flags.add(PendingConfigureAck);
    role.imp()->configureSerial = serial;

    bool notifyStates { role.imp()->flags.check(PendingFirstConfigure) };
    bool notifySuggestedSize { notifyStates };
    role.imp()->flags.remove(PendingFirstConfigure);

    const SkISize suggestedSize (width, height);

    if (role.suggestedSize() != suggestedSize )
    {
        role.imp()->suggestedSize = suggestedSize;
        notifySuggestedSize = true;
    }

    if (notifySuggestedSize)
    {
        role.suggestedSizeChanged();
    }

    if (notifyStates)
    {
        role.imp()->flags.add(ForceUpdate);
        role.update();
    }
}

void MLayerSurface::Imp::closed(void *data, zwlr_layer_surface_v1 *layerSurface)
{

}


MLayerSurface::Imp::Imp(MLayerSurface &obj) noexcept : obj(obj)
{
    layerSurfaceListener.configure = &configure;
    layerSurfaceListener.closed = &closed;
}
