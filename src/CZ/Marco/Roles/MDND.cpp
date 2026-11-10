#include <CZ/Marco/Roles/MDND.h>
#include <CZ/Marco/Private/MSurfacePrivate.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Ream/WL/RWLSwapchain.h>
#include <CZ/Core/Events/CZInputEvent.h>
#include <CZ/Core/CZWeak.h>
#include <unistd.h>

using namespace CZ;

const wl_data_source_listener MDND::Listener
{
    .target = MDND::source_target,
    .send = MDND::source_send,
    .cancelled = MDND::source_cancelled,
    .dnd_drop_performed = MDND::source_dnd_drop_performed,
    .dnd_finished = MDND::source_dnd_finished,
    .action = MDND::source_action
};

MDND::MDND() noexcept : MSurface(Role::DNDIcon)
{
    setColor(CZAdaptiveColor(0x00000000));

    // Purely visual (the compositor ignores the icon's input anyway).
    SkRegion empty;
    setCustomInputRegion(&empty);
}

MDND::~MDND() noexcept
{
    if (m_source)
    {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }
}

bool MDND::start(MSurface &origin, const CZInputEvent &event, const std::vector<std::string> &mimeTypes, CZBitset<Action> actions) noexcept
{
    auto app { MApp::Get() };

    if (m_source)
    {
        MLog(CZWarning, CZLN, "A drag & drop session is already active");
        return false;
    }

    if (!app->wl.dataDeviceManager || !app->wl.dataDevice)
    {
        MLog(CZWarning, CZLN, "Drag & drop not supported by the compositor");
        return false;
    }

    m_source = wl_data_device_manager_create_data_source(app->wl.dataDeviceManager);
    wl_data_source_add_listener(m_source, &Listener, this);

    for (const auto &mime : mimeTypes)
        wl_data_source_offer(m_source, mime.c_str());

    if (wl_data_source_get_version(m_source) >= 3)
        wl_data_source_set_actions(m_source, actions.get());

    m_accepted.clear();
    m_action = Action::NoAction;

    // Offsets are relative and the surface keeps its role (and the compositor its hotspot) across
    // sessions, so m_sentHotspot carries over.

    // The icon is composited at the origin's scale; don't wait for preferred_buffer_scale.
    if (imp()->scale != origin.scale())
    {
        imp()->scale = origin.scale();
        imp()->tmpFlags.add(Imp::ScaleChanged);
    }

    wl_data_device_start_drag(app->wl.dataDevice, m_source, origin.wlSurface(), wlSurface(), event.serial);
    setMapped(true);
    update(true);
    return true;
}

void MDND::cancel() noexcept
{
    if (!m_source)
        return;

    end();
    onCancelled.notify();
}

void MDND::setHotspot(const SkIPoint &hotspot) noexcept
{
    if (m_hotspot == hotspot)
        return;

    m_hotspot = hotspot;
    update(true);
}

void MDND::end() noexcept
{
    if (m_source)
    {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }

    setMapped(false);
}

void MDND::onUpdate() noexcept
{
    MSurface::onUpdate();

    if (!m_source || !imp()->flags.has(Imp::UserMapped))
    {
        if (imp()->flags.has(Imp::HasBufferAttached))
        {
            wl_surface_attach(wlSurface(), nullptr, 0, 0);
            wl_surface_commit(wlSurface());
            imp()->flags.remove(Imp::HasBufferAttached);
        }

        imp()->setMapped(false);
        return;
    }

    imp()->setMapped(true);
    render();
}

void MDND::render() noexcept
{
    scene().root()->layout().calculate();

    if (!ShouldUpdate(*this))
        return;

    // The hotspot is set through relative offsets (the icon is moved so the pointer stays on it).
    if (m_hotspot != m_sentHotspot && wl_surface_get_version(wlSurface()) >= WL_SURFACE_OFFSET_SINCE_VERSION)
    {
        wl_surface_offset(wlSurface(), m_sentHotspot.x() - m_hotspot.x(), m_sentHotspot.y() - m_hotspot.y());
        m_sentHotspot = m_hotspot;
    }

    const SkISize newSize { CalculatedSize(*this) };
    const bool sizeChanged { imp()->resizeBuffer(newSize) };
    const bool repaint { !imp()->flags.has(Imp::HasBufferAttached) || sizeChanged || target()->isDirty() || target()->bakedNodesScale() != scale() };

    if (!repaint)
    {
        wl_surface_commit(wlSurface());
        return;
    }

    SkRegion outDamage, outOpaque, outInvisible;
    auto ssImage { imp()->swapchain->acquire() };
    PrepareTarget(*this, ssImage.value(), &outDamage, &outOpaque, &outInvisible, sizeChanged);
    scene().render(target());
    AttachInputRegion(*this);
    AttachOpaqueRegion(*this, outOpaque);
    PresentImage(*this, *ssImage, outDamage);
}

void MDND::source_target(void *data, wl_data_source */*source*/, const char *mimeType)
{
    static_cast<MDND*>(data)->m_accepted = mimeType ? mimeType : "";
}

void MDND::source_send(void *data, wl_data_source */*source*/, const char *mimeType, Int32 fd)
{
    auto &dnd { *static_cast<MDND*>(data) };

    if (dnd.onSend)
        dnd.onSend(mimeType, fd);

    close(fd);
}

void MDND::source_cancelled(void *data, wl_data_source */*source*/)
{
    CZWeak<MDND> dnd { static_cast<MDND*>(data) };
    dnd->end();
    dnd->onCancelled.notify();
}

void MDND::source_dnd_drop_performed(void *data, wl_data_source */*source*/)
{
    static_cast<MDND*>(data)->onDropPerformed.notify();
}

void MDND::source_dnd_finished(void *data, wl_data_source */*source*/)
{
    CZWeak<MDND> dnd { static_cast<MDND*>(data) };
    dnd->end();
    dnd->onFinished.notify();
}

void MDND::source_action(void *data, wl_data_source */*source*/, UInt32 action)
{
    static_cast<MDND*>(data)->m_action = static_cast<Action>(action);
}
