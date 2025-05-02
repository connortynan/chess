#include "chess/game.hpp"
#include "chess/move.hpp"
#include "ui/ui.hpp"

#ifdef DEBUG
#include "chess/debug.hpp"
#endif

int main()
{
    chess::Game game;

    ui::init(&game);

    while (ui::is_running())
    {
        ui::update();
        ui::display();
        ui::wait_for_input();
    }

    ui::cleanup();

    return 0;
}