#ifndef MCLIPBOARD_H
#define MCLIPBOARD_H

#include <CZ/AK/AKClipboard.h>
#include <CZ/Core/CZEventSource.h>
#include <wayland-client-protocol.h>
#include <list>
#include <memory>

namespace CZ
{
/**
 * @brief Kay's clipboard (AKApp::clipboard()) over the Wayland data device (wl_data_device
 *        selection), set by MApp.
 */
class MClipboard final : public AKClipboard
{
public:
    ~MClipboard() noexcept;
    void setText(const std::string &text, const CZInputEvent &triggeringEvent) noexcept override;
    void requestText(std::function<void(std::optional<std::string> text)> done) noexcept override;
    void setData(const std::vector<Format> &formats, const CZInputEvent &triggeringEvent) noexcept override;
    void requestData(const std::vector<std::string> &mimeTypes, std::function<void(std::optional<Format> data)> done) noexcept override;
    std::vector<std::string> mimeTypes() const noexcept override;

private:
    struct Read
    {
        std::shared_ptr<CZEventSource> source;
        std::string data;
        std::string mimeType;
        std::function<void(std::optional<Format>)> done;
        bool finished { false };
    };

    void finishRead(Read *read, bool ok) noexcept;
    void clearSource() noexcept;

    static void source_target(void*, wl_data_source*, const char*) {}
    static void source_send(void *data, wl_data_source *source, const char *mimeType, Int32 fd);
    static void source_cancelled(void *data, wl_data_source *source);
    static void source_dnd_drop_performed(void*, wl_data_source*) {}
    static void source_dnd_finished(void*, wl_data_source*) {}
    static void source_action(void*, wl_data_source*, UInt32) {}
    static const wl_data_source_listener SourceListener;

    wl_data_source *m_source {}; // Ours while we own the selection
    std::vector<Format> m_formats; // What we offer while we own the selection
    std::list<std::unique_ptr<Read>> m_reads;
};
}

#endif // MCLIPBOARD_H
