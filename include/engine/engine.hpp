#pragma once

#include "chess/game.hpp"

namespace chess
{
    namespace engine
    {

        Move solve(Game &game, int depth, int *eval_centipawns = nullptr);

    } // namespace engine
} // namespace chess