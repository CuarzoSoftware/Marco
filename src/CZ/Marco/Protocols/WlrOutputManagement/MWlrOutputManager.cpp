#include <CZ/Marco/Protocols/WlrOutputManagement/MWlrOutputManager.h>
#include <CZ/Marco/Extensions/MOutputManager.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <algorithm>

using namespace CZ;

static MOutputHead *Head(zwlr_output_head_v1 *head) noexcept
{
    return static_cast<MOutputHead*>(zwlr_output_head_v1_get_user_data(head));
}

static MOutputMode *Mode(zwlr_output_mode_v1 *mode) noexcept
{
    return mode ? static_cast<MOutputMode*>(zwlr_output_mode_v1_get_user_data(mode)) : nullptr;
}

/* ------------------------------------------ Manager ------------------------------------------ */

void MWlrOutputManager::head(void */*data*/, zwlr_output_manager_v1 */*manager*/, zwlr_output_head_v1 *head)
{
    auto *res { new MOutputHead(head) };
    zwlr_output_head_v1_add_listener(head, &MWlrOutputHead::Listener, res);
    MApp::Get()->ext.outputManager.m_incomplete.emplace_back(res);

    // UAPI is notified after the manager's done event
}

void MWlrOutputManager::done(void */*data*/, zwlr_output_manager_v1 */*manager*/, UInt32 serial)
{
    auto app { MApp::Get() };
    auto &manager { app->ext.outputManager };
    manager.m_serial = serial;
    manager.m_hasSerial = true;

    // Changes of the known ones first, then the new ones
    for (auto *head : std::vector<MOutputHead*>(manager.m_heads))
        head->commit();

    const auto added { std::move(manager.m_incomplete) };
    manager.m_incomplete.clear();

    for (auto *head : added)
    {
        head->commit();
        manager.m_heads.emplace_back(head);
        MLog(CZTrace, "Output head added: {}", head->props().name);
        manager.onAdded.notify(head);
    }

    for (auto *head : manager.m_heads)
        head->destroyFinishedModes();

    manager.onChanged.notify();
}

void MWlrOutputManager::finished(void */*data*/, zwlr_output_manager_v1 *manager)
{
    auto app { MApp::Get() };
    app->ext.outputManager.unit();
    zwlr_output_manager_v1_destroy(manager);
    app->wl.outputManager.reset();
}

/* -------------------------------------------- Head ------------------------------------------- */

void MWlrOutputHead::name(void */*data*/, zwlr_output_head_v1 *head, const char *name)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHName);
    res->m_pending.name = name;
}

void MWlrOutputHead::description(void */*data*/, zwlr_output_head_v1 *head, const char *description)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHDescription);
    res->m_pending.description = description;
}

void MWlrOutputHead::physical_size(void */*data*/, zwlr_output_head_v1 *head, Int32 width, Int32 height)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHPhysicalSize);
    res->m_pending.physicalSize = SkISize(width, height);
}

void MWlrOutputHead::mode(void */*data*/, zwlr_output_head_v1 *head, zwlr_output_mode_v1 *mode)
{
    auto *res { Head(head) };
    auto *m { new MOutputMode(*res, mode) };
    zwlr_output_mode_v1_add_listener(mode, &MWlrOutputMode::Listener, m);
    res->m_modes.emplace_back(m);
    res->m_pending.modes.emplace_back(m);
    res->m_changes.add(MOutputHead::CHModes);
}

void MWlrOutputHead::enabled(void */*data*/, zwlr_output_head_v1 *head, Int32 enabled)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHEnabled);
    res->m_pending.enabled = enabled != 0;

    // Mode, position, transform and scale are only sent while enabled
    if (!enabled)
    {
        res->m_changes.add(MOutputHead::CHCurrentMode);
        res->m_pending.currentMode = nullptr;
    }
}

void MWlrOutputHead::current_mode(void */*data*/, zwlr_output_head_v1 *head, zwlr_output_mode_v1 *mode)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHCurrentMode);
    res->m_pending.currentMode = Mode(mode);
}

void MWlrOutputHead::position(void */*data*/, zwlr_output_head_v1 *head, Int32 x, Int32 y)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHPos);
    res->m_pending.pos = SkIPoint(x, y);
}

void MWlrOutputHead::transform(void */*data*/, zwlr_output_head_v1 *head, Int32 transform)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHTransform);
    res->m_pending.transform = static_cast<CZTransform>(transform);
}

void MWlrOutputHead::scale(void */*data*/, zwlr_output_head_v1 *head, wl_fixed_t scale)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHScale);
    res->m_pending.scale = wl_fixed_to_double(scale);
}

void MWlrOutputHead::finished(void */*data*/, zwlr_output_head_v1 *head)
{
    MApp::Get()->ext.outputManager.removeHead(Head(head));
}

void MWlrOutputHead::make(void */*data*/, zwlr_output_head_v1 *head, const char *make)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHMake);
    res->m_pending.make = make;
}

void MWlrOutputHead::model(void */*data*/, zwlr_output_head_v1 *head, const char *model)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHModel);
    res->m_pending.model = model;
}

void MWlrOutputHead::serial_number(void */*data*/, zwlr_output_head_v1 *head, const char *serialNumber)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHSerialNumber);
    res->m_pending.serialNumber = serialNumber;
}

void MWlrOutputHead::adaptive_sync(void */*data*/, zwlr_output_head_v1 *head, UInt32 state)
{
    auto *res { Head(head) };
    res->m_changes.add(MOutputHead::CHAdaptiveSync);
    res->m_pending.adaptiveSync = state == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
}

/* -------------------------------------------- Mode ------------------------------------------- */

void MWlrOutputMode::size(void */*data*/, zwlr_output_mode_v1 *mode, Int32 width, Int32 height)
{
    Mode(mode)->m_size = SkISize(width, height);
}

void MWlrOutputMode::refresh(void */*data*/, zwlr_output_mode_v1 *mode, Int32 refresh)
{
    Mode(mode)->m_refreshRate = refresh;
}

void MWlrOutputMode::preferred(void */*data*/, zwlr_output_mode_v1 *mode)
{
    Mode(mode)->m_preferred = true;
}

void MWlrOutputMode::finished(void */*data*/, zwlr_output_mode_v1 *mode)
{
    // Taken out of its head now, destroyed once the head is done (see MOutputHead::destroyFinishedModes())
    auto *m { Mode(mode) };
    auto &head { m->head() };
    m->m_finished = true;
    std::erase(head.m_pending.modes, m);
    head.m_changes.add(MOutputHead::CHModes);

    if (head.m_pending.currentMode == m)
    {
        head.m_pending.currentMode = nullptr;
        head.m_changes.add(MOutputHead::CHCurrentMode);
    }
}

/* ---------------------------------------- Configuration -------------------------------------- */

void MWlrOutputConfiguration::succeeded(void *data, zwlr_output_configuration_v1 */*configuration*/)
{
    MApp::Get()->ext.outputManager.finish(static_cast<MOutputManager::Configuration*>(data), MOutputManager::Result::Succeeded);
}

void MWlrOutputConfiguration::failed(void *data, zwlr_output_configuration_v1 */*configuration*/)
{
    MApp::Get()->ext.outputManager.finish(static_cast<MOutputManager::Configuration*>(data), MOutputManager::Result::Failed);
}

void MWlrOutputConfiguration::cancelled(void *data, zwlr_output_configuration_v1 */*configuration*/)
{
    MApp::Get()->ext.outputManager.finish(static_cast<MOutputManager::Configuration*>(data), MOutputManager::Result::Cancelled);
}
