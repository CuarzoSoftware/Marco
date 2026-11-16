#ifndef MSESSIONLOCKSURFACE_H
#define MSESSIONLOCKSURFACE_H

#include <CZ/Marco/Roles/MSurface.h>

/**
 * @brief A surface shown on a screen while the session is locked (ext-session-lock-v1).
 *
 * See MSessionLockManager. Create one per screen (and for screens plugged later), set its screen
 * and map it: it's shown only while the session lock is Locking or Locked, unmapped otherwise
 * (mapped() follows that). It always covers its whole screen: the compositor decides its size
 * (suggestedSize(), applied to its layout by suggestedSizeChanged()).
 *
 * Only one per screen: another one set to the same screen isn't shown (an error is logged) until
 * the first one leaves it. When its screen is unplugged it's unmapped (screen() becomes nullptr).
 *
 * Nothing is shown if the compositor doesn't support the protocol (see MApp::sessionLockManager()).
 */
class CZ::MSessionLockSurface : public MSurface
{
public:
    MSessionLockSurface(MScreen *screen = nullptr) noexcept;
    ~MSessionLockSurface() noexcept;

    bool setScreen(MScreen *screen) noexcept;
    MScreen *screen() const noexcept;

    /// The size the compositor requires (its screen's), (0, 0) until configured.
    const SkISize &suggestedSize() const noexcept;

    class Imp;
    Imp *imp() const noexcept;

protected:
    /// Applies suggestedSize() to the layout (by default).
    virtual void suggestedSizeChanged();
    virtual void render() noexcept;
    void onUpdate() noexcept override;

private:
    std::unique_ptr<Imp> m_imp;
};

#endif // MSESSIONLOCKSURFACE_H
