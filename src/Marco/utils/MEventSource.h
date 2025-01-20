#ifndef MEVENTSOURCE_H
#define MEVENTSOURCE_H

#include <Marco/Marco.h>
#include <AK/AKObject.h>
#include <functional>

class Marco::MEventSource : public AK::AKObject
{
public:
    AKCLASS_NO_COPY(MEventSource);
    using Callback = std::function<void(Int32, UInt32)>;
    Int32 fd() const noexcept { return m_fd; }
    UInt32 events() const noexcept { return m_events; }
    ~MEventSource() = default;
private:
    friend class MApplication;
    MEventSource(Int32 fd, UInt32 events, const Callback &callback) noexcept :
        m_fd(fd),
        m_events(events),
        m_callback(callback) {}
    Int32 m_fd;
    UInt32 m_events;
    Callback m_callback;
};

#endif // MEVENTSOURCE_H
