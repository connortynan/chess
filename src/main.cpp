// #include <iostream>
// #include <bitset>
// #include <string>

// #include "chess/game.hpp"
// #include "chess/move.hpp"
// #include "ui/ui.hpp"

// #ifdef DEBUG
// #include "chess/debug.hpp"
// #endif

// // todo: BAD
// namespace chess
// {
//     void init_attacks();
// }

// int main()
// {
//     chess::Game game;

//     ui::init(&game);
//     chess::init_attacks();

//     while (ui::is_running())
//     {
//         ui::update();
//         ui::display();
//         ui::wait_for_input();
//     }
//     ui::display();

//     ui::cleanup();

//     return 0;
// }

#include <iostream>
#include <string>

#include "chess/position.hpp"
#include "chess/move.hpp"
#include "engine/engine.hpp"

namespace chess
{
    void init_attacks(); // still needed to set up attack tables
}

int main()
{
    using namespace chess;

    init_attacks();

    Position pos;
    pos.from_fen(default_fen); // standard starting position

    std::cout << "Evaluating position: " << pos.to_fen() << "\n";

    int eval_cp = 0;
    Move best = engine::solve(pos, /*depth=*/3, &eval_cp);

    std::cout << "Best move: " << move::to_string(best) << "\n";
    std::cout << "Eval: " << eval_cp << " centipawns\n";

    return 0;
}
