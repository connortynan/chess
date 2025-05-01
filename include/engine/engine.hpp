#pragma once

#include "chess/position.hpp"

namespace chess
{
    namespace engine
    {
        Move solve(const Position &pos, int depth, int *eval_cp = nullptr);

    } // namespace engine
} // namespace chess