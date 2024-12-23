#include <Marco/roles/MSurface.h>
#include <Marco/MApplication.h>
#include <AK/AKColors.h>

using namespace Marco;

MSurface::~MSurface()
{
    app()->m_surfaces[m_appLink] = app()->m_surfaces.back();
    app()->m_surfaces.pop_back();
    m_scene.destroyTarget(m_target);
}

MSurface::MSurface(Role role) noexcept : AK::AKSolidColor(AK::AKColor::GrayLighten5), m_role(role)
{
    m_appLink = app()->m_surfaces.size();
    app()->m_surfaces.push_back(this);
    setParent(&m_root);
    setClipsChildren(true);
    m_target.reset(m_scene.createTarget());
    m_target->root = &m_root;
    m_changes.set(CHSize);
}
