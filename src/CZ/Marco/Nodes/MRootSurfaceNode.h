#ifndef MROOTSURFACENODE_H
#define MROOTSURFACENODE_H

#include <CZ/Marco/Marco.h>
#include <CZ/AK/Nodes/AKContainer.h>

/**
 * @brief The root node of an MSurface (see MSurface::rootNode()), created by the surface itself.
 *
 * Parent of the surface's central node and its decorations.
 */
class CZ::MRootSurfaceNode : public AKContainer
{
public:
    /** @brief Creates the root node of @p surface. */
    MRootSurfaceNode(MSurface &surface) noexcept :
        m_surface(surface) {}

    /** @brief The surface this node belongs to. */
    MSurface &surface() const noexcept
    {
        return m_surface;
    }
private:
    MSurface &m_surface;
};

#endif // MROOTSURFACENODE_H
