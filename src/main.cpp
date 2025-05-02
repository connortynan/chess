#include "chess/game.hpp"
#include "chess/move.hpp"
#include "ui/ui.hpp"

#ifdef DEBUG
#include "chess/debug.hpp"
#endif

// todo: BAD
namespace chess
{
    void init_attacks();
}

int main()
{
    chess::Game game;

    ui::init(&game);
    chess::init_attacks();

    while (ui::is_running())
    {
        ui::update();
        ui::display();
        ui::wait_for_input();
    }

    ui::cleanup();

    return 0;
}

// #include <iostream>
// #include <string>

// #include "chess/position.hpp"
// #include "chess/move.hpp"
// #include "engine/engine.hpp"

// namespace chess
// {
//     void init_attacks(); // still needed to set up attack tables
// }

// int main()
// {
//     using namespace chess;

//     init_attacks();

//     Position pos;
//     pos.from_fen(default_fen); // standard starting position

//     std::cout << "Evaluating position: " << pos.to_fen() << "\n";

//     int eval_cp = 0;
//     Move best = engine::solve(pos, /*depth=*/3, &eval_cp);

//     std::cout << "Best move: " << move::to_string(best) << "\n";
//     std::cout << "Eval: " << eval_cp << " centipawns\n";

//     return 0;
// }

// #include <iostream>

// #include "chess/position.hpp"
// #include "engine/engine.hpp"

// int main(int argc, char const *argv[])
// {
//     chess::Position pos;
//     pos.from_fen(chess::default_fen); // standard starting position

//     int score_white;
//     int score_black;

//     chess::engine::solve(pos, 6, &score_white); // White to move

//     pos.from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1"); // swap turn
//     chess::engine::solve(pos, 6, &score_black);                               // Black to move

//     std::cout << "White: " << score_white << "\n";
//     std::cout << "Black: " << score_black << std::endl;

//     return 0;
// }

// #include <iostream>
// #include "chess/position.hpp"
// #include "engine/engine.hpp"

// int main(int argc, char const *argv[])
// {
//     chess::init_attacks();

//     chess::Position pos;
//     pos.from_fen("6k1/8/5K2/5Q2/8/8/8/8 w - - 0 1"); // White can play Qg7#

//     chess::Move move = chess::engine::solve(pos, 6);
//     std::cout << "Best move: " << chess::move::to_string(move) << "\n";

//     return 0;
// }
