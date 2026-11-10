#ifndef MMENUDIVIDER_H
#define MMENUDIVIDER_H

#include <CZ/Marco/Nodes/MMenuItem.h>
#include <CZ/AK/Nodes/AKSolidColor.h>

/**
 * @brief A horizontal separator between menu rows. Not selectable.
 */
class CZ::MMenuDivider : public MMenuItem
{
public:
    MMenuDivider(AKNode *parent = nullptr) noexcept;

    bool enabled() const noexcept override { return false; }

    AKSolidColor line;
};

#endif // MMENUDIVIDER_H
