#include <CZ/Marco/Private/MClipboard.h>
#include <CZ/Marco/Protocols/Wayland/MWlDataDevice.h>
#include <CZ/Marco/MDNDOffer.h>
#include <CZ/Marco/MApp.h>
#include <CZ/Marco/MLog.h>
#include <CZ/Core/CZTimer.h>
#include <CZ/Core/Events/CZInputEvent.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <algorithm>

using namespace CZ;

static constexpr const char *TextMimeTypes[] { "text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "TEXT", "STRING" };
static constexpr size_t MaxPasteBytes { 16 * 1024 * 1024 };

const wl_data_source_listener MClipboard::SourceListener
{
    .target = source_target,
    .send = source_send,
    .cancelled = source_cancelled,
    .dnd_drop_performed = source_dnd_drop_performed,
    .dnd_finished = source_dnd_finished,
    .action = source_action
};

MClipboard::~MClipboard() noexcept
{
    clearSource();
}

void MClipboard::clearSource() noexcept
{
    if (m_source)
    {
        wl_data_source_destroy(m_source);
        m_source = nullptr;
    }

    m_formats.clear();
}

void MClipboard::setText(const std::string &text, const CZInputEvent &triggeringEvent) noexcept
{
    std::vector<Format> formats;

    for (const char *mimeType : TextMimeTypes)
        formats.emplace_back(mimeType, text);

    setData(formats, triggeringEvent);
}

void MClipboard::setData(const std::vector<Format> &formats, const CZInputEvent &triggeringEvent) noexcept
{
    auto app { MApp::Get() };

    if (!app || !app->wl.dataDeviceManager || !app->wl.dataDevice || formats.empty())
        return;

    clearSource();
    m_formats = formats;
    m_source = wl_data_device_manager_create_data_source(app->wl.dataDeviceManager);
    wl_data_source_add_listener(m_source, &SourceListener, this);

    for (const auto &[mimeType, data] : m_formats)
        wl_data_source_offer(m_source, mimeType.c_str());

    wl_data_device_set_selection(app->wl.dataDevice, m_source, triggeringEvent.serial);
    wl_display_flush(app->wl.display);
}

std::vector<std::string> MClipboard::mimeTypes() const noexcept
{
    std::vector<std::string> types;

    if (m_source)
    {
        for (const auto &[mimeType, data] : m_formats)
            types.emplace_back(mimeType);
    }
    else if (MDNDOffer *offer { MWlDataDevice::SelectionOffer() })
        types = offer->mimeTypes();

    return types;
}

void MClipboard::source_send(void *data, wl_data_source */*source*/, const char *mimeType, Int32 fd)
{
    auto &self { *static_cast<MClipboard*>(data) };
    const auto format { std::find_if(self.m_formats.begin(), self.m_formats.end(),
        [mimeType](const Format &f){ return mimeType && f.first == mimeType; }) };
    const std::string empty;
    const std::string &bytes { format != self.m_formats.end() ? format->second : empty };

    // Blocking (short) writes: the reader is another process
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    size_t written { 0 };

    while (written < bytes.size())
    {
        const ssize_t n { write(fd, bytes.data() + written, bytes.size() - written) };

        if (n < 0 && errno == EINTR)
            continue;

        if (n <= 0)
            break;

        written += size_t(n);
    }

    close(fd);
}

void MClipboard::source_cancelled(void *data, wl_data_source *source)
{
    // Someone else owns the selection now
    auto &self { *static_cast<MClipboard*>(data) };

    if (self.m_source == source)
        self.clearSource();
    else
        wl_data_source_destroy(source);
}

void MClipboard::requestText(std::function<void(std::optional<std::string>)> done) noexcept
{
    requestData(std::vector<std::string>(std::begin(TextMimeTypes), std::end(TextMimeTypes)),
        [done = std::move(done)](std::optional<Format> data){
            done(data ? std::optional(std::move(data->second)) : std::nullopt);
        });
}

void MClipboard::requestData(const std::vector<std::string> &mimeTypes, std::function<void(std::optional<Format>)> done) noexcept
{
    // Ours (also avoids reading from our own pipe), or nothing to read: answered from the loop
    const auto answerLater = [&done](std::optional<Format> data) {
        CZTimer::OneShot(0, [done = std::move(done), data = std::move(data)](CZTimer*) { done(data); });
    };

    if (m_source)
    {
        for (const auto &type : mimeTypes)
            for (const auto &format : m_formats)
                if (format.first == type)
                {
                    answerLater(format);
                    return;
                }

        answerLater(std::nullopt);
        return;
    }

    MDNDOffer *offer { MWlDataDevice::SelectionOffer() };

    if (!offer)
    {
        answerLater(std::nullopt);
        return;
    }

    const std::string *mimeType { nullptr };

    for (const auto &candidate : mimeTypes)
        if (offer->hasMimeType(candidate)) { mimeType = &candidate; break; }

    const Int32 fd { mimeType ? offer->receive(*mimeType) : -1 };

    if (fd < 0)
    {
        answerLater(std::nullopt);
        return;
    }

    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    auto read { std::make_unique<Read>() };
    read->done = std::move(done);
    read->mimeType = *mimeType;
    Read *raw { read.get() };

    read->source = CZEventSource::Make(fd, EPOLLIN, CZOwn::Own, [this, raw](int fd, UInt32, CZEventSource*) {
        if (raw->finished)
            return;

        char buf[4096];

        while (true)
        {
            const ssize_t n { ::read(fd, buf, sizeof(buf)) };

            if (n > 0)
            {
                raw->data.append(buf, size_t(n));

                if (raw->data.size() > MaxPasteBytes)
                {
                    finishRead(raw, false);
                    return;
                }
                continue;
            }

            if (n < 0 && errno == EINTR)
                continue;

            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                return; // More later

            finishRead(raw, n == 0); // EOF (or an error)
            return;
        }
    });

    m_reads.emplace_back(std::move(read));
}

void MClipboard::finishRead(Read *read, bool ok) noexcept
{
    // Destroyed out of its event source's callback
    read->finished = true;

    CZTimer::OneShot(0, [this, read, ok](CZTimer*) {
        auto it { std::find_if(m_reads.begin(), m_reads.end(), [read](const auto &r){ return r.get() == read; }) };

        if (it == m_reads.end())
            return;

        auto owned { std::move(*it) };
        m_reads.erase(it);

        if (owned->done)
            owned->done(ok ? std::optional(Format { std::move(owned->mimeType), std::move(owned->data) }) : std::nullopt);
    });
}
