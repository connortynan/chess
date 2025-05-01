#include <iostream>
#include <bitset>
#include <string>

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
    ui::display();

    ui::cleanup();

    return 0;
}