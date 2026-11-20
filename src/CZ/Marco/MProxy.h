#ifndef MPROXY_H
#define MPROXY_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/AKObject.h>
#include <wayland-client.h>

/**
 * @brief A Wayland proxy of type @p T, with the registry name of the global it was bound from.
 *
 * Converts implicitly to `T*`, so it can be passed directly to the protocol functions. Holds a
 * null proxy (and name 0) when unset. It doesn't own the proxy: destroying it doesn't destroy the
 * Wayland object.
 */
template<class T>
class CZ::MProxy : public AKObject
{
public:
    /** @brief Wraps @p proxy, optionally bound from the global @p name. */
    MProxy(void *proxy = nullptr, UInt32 name = 0) noexcept :
        m_proxy(proxy),
        m_name(name){}

    /** @brief The proxy's interface version, 0 if unset. */
    UInt32 version() noexcept
    {
        return m_proxy ? wl_proxy_get_version((wl_proxy*)m_proxy) : 0;
    }

    /** @brief Replaces the proxy and its global name (the previous proxy isn't destroyed). */
    void set(void *proxy, UInt32 name = 0) noexcept
    {
        m_proxy = proxy;
        m_name = name;
    }

    /** @brief The proxy, or `nullptr`. */
    T *get() const noexcept
    {
        return static_cast<T*>(m_proxy);
    }

    /** @brief Sets the registry name of the global. */
    void setName(UInt32 name) noexcept
    {
        m_name = name;
    }

    /** @brief The registry name of the global, 0 if unknown. */
    UInt32 name() const noexcept
    {
        return m_name;
    }

    /** @brief Same as get(). */
    operator T *() const noexcept
    {
        return get();
    }

    /** @brief Clears the proxy and name (the proxy isn't destroyed). */
    void reset() noexcept
    {
        m_proxy = nullptr;
        m_name = 0;
    }
private:
    void *m_proxy;
    UInt32 m_name;
};

#endif // MPROXY_H
