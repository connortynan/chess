#include "ui/ui.hpp"

#include <array>
#include <list>
#include <ncurses.h>
#include <cassert>

#include "chess/inttypes.hpp"
#include "chess/position.hpp"
#include "engine/engine.hpp"
#include "commands.hpp"

static constexpr int ENGINE_DEPTH = 7;

namespace ui
{
    namespace color_pairs
    {
        // Lower bit: light/dark (plus a higher always on bit to get out of default ncurses range)
        constexpr short LIGHT = 0b1000;
        constexpr short DARK = 0b1001;

        // Tint types
        constexpr short TINT_NORMAL = 0 << 1;
        constexpr short TINT_RED = 1 << 1;
        constexpr short TINT_GREEN = 2 << 1;
        constexpr short TINT_YELLOW = 3 << 1;
        constexpr short TINT_MASK = 0b110;

        // Composite styles
        constexpr short NORMAL_LIGHT = TINT_NORMAL | LIGHT; // 0b000
        constexpr short NORMAL_DARK = TINT_NORMAL | DARK;   // 0b001
        constexpr short RED_LIGHT = TINT_RED | LIGHT;       // 0b010
        constexpr short RED_DARK = TINT_RED | DARK;         // 0b011
        constexpr short GREEN_LIGHT = TINT_GREEN | LIGHT;   // 0b100
        constexpr short GREEN_DARK = TINT_GREEN | DARK;     // 0b101
        constexpr short YELLOW_LIGHT = TINT_YELLOW | LIGHT; // 0b110
        constexpr short YELLOW_DARK = TINT_YELLOW | DARK;   // 0b111

        constexpr short CYAN_GRAY = 0b10000; // 0b1000
    }

    enum Window : int
    {
        W_BOARD,
        W_SIDEBAR,
        W_DIALOGUE,

        W_COUNT
    };

    constexpr int BOARD_ASPECT_X = 7;
    constexpr int BOARD_ASPECT_Y = 3;

    // [Window][h, w, y, x]
    constexpr int WINDOW_SIZES[W_COUNT][4] = {
        // W_BOARD
        {8 * BOARD_ASPECT_Y, 8 * BOARD_ASPECT_X,
         24 - 8 * BOARD_ASPECT_Y, 80 - 8 * BOARD_ASPECT_X},

        // W_SIDEBAR
        {24, 80 - 8 * BOARD_ASPECT_X, 0, 0},

        // W_DIALOGUE
        {14, 50, 5, 15},
    };

    struct Cell
    {
        char piece = ' ';
        short style = color_pairs::NORMAL_LIGHT;
    };

    static const std::string piece_names = "PNBRQKpnbrqk";

    class State
    {
    public:
        std::array<WINDOW *, W_COUNT> windows;
        std::array<Cell, 64> board_cells;
        std::array<std::list<chess::Move>, 64> tile_moves;
        std::vector<std::string> move_text;
        bool flip_board = false;
        chess::Move moves[256];
        int move_count = 0;

        chess::Game *game = nullptr;
        bool owns_game = false;

        int cursor_square = 0;
        int selected_square = -1;
        int check_square = -1;
        int last_move_from = -1;
        int last_move_to = -1;

        int term_x = 0;
        int term_y = 0;

        bool running = true;
        std::string status = "";

        bool white_engine = false;
        bool black_engine = false;

        enum Mode
        {
            MD_STARTUP,
            MD_SELECT,
            MD_SHOW_MOVES,
            MD_GAME_OVER,
            MD_HELP,
        } mode = MD_STARTUP;

        Mode last_mode;

        ~State()
        {
            for (int i = 0; i < W_COUNT; ++i)
            {
                delwin(windows[i]);
            }
            endwin();
        }

        void init()
        {
            for (int i = 0; i < 64; ++i)
            {
                board_cells[i].style = ((i + (i / 8)) % 2) ? color_pairs::NORMAL_LIGHT : color_pairs::NORMAL_DARK;
            }
            for (int i = 0; i < W_COUNT; ++i)
            {
                windows[i] = newwin(WINDOW_SIZES[i][0], WINDOW_SIZES[i][1], WINDOW_SIZES[i][2], WINDOW_SIZES[i][3]);
            }

            wbkgd(windows[W_SIDEBAR], COLOR_PAIR(color_pairs::CYAN_GRAY));
            wbkgd(windows[W_DIALOGUE], COLOR_PAIR(color_pairs::CYAN_GRAY));
            bkgd(COLOR_PAIR(color_pairs::CYAN_GRAY));
            check_resize();
        }

        void check_resize()
        {
            int rows, cols;
            getmaxyx(stdscr, rows, cols);

            if (term_x == cols && term_y == rows)
                return;

            term_x = cols;
            term_y = rows;

            int offset_y = std::max(0, (rows - 24) / 2); // 24 = full UI height
            int offset_x = std::max(0, (cols - 80) / 2); // 80 = full UI width

            for (int i = 0; i < W_COUNT; ++i)
            {
                int h = WINDOW_SIZES[i][0];
                int w = WINDOW_SIZES[i][1];
                int y = WINDOW_SIZES[i][2];
                int x = WINDOW_SIZES[i][3];

                mvwin(windows[i], offset_y + y, offset_x + x);
                wresize(windows[i], h, w);
            }

            clear();
            refresh();

            display();
        }

        void update()
        {
            const chess::Position &pos = game->position;
            assert(pos.validate_occupancy());

            // board
            for (int sq = 0; sq < 64; ++sq) // clear all cells
                board_cells[sq].piece = ' ';

            for (int c = 0; c < 2; ++c)
            {
                for (int pt = 0; pt < 6; ++pt)
                {
                    char piece = piece_names[c * 6 + pt];
                    for (int sq = 0; sq < 64; ++sq)
                    {
                        if (pos.pieces[c][pt] & (1ULL << sq))
                        {
                            board_cells[sq].piece = piece;
                        }
                    }
                }
            }

            // moves
            tile_moves.fill({});
            move_count = game->get_moves(moves);

            for (int i = 0; i < move_count; ++i)
            {
                chess::Move m = moves[i];
                int from = chess::move::from(m);
                tile_moves[from].push_back(m);
            }
        }

        void highlight_square(int square, short tint)
        {
            assert(square >= 0 && square < 64);
            assert(tint >= color_pairs::TINT_NORMAL && tint <= color_pairs::TINT_YELLOW);

            short style = (board_cells[square].style & ~color_pairs::TINT_MASK) | tint;
            board_cells[square].style = style;
        }

        void draw_board()
        {
            WINDOW *board = windows[W_BOARD];
            for (int y = 0; y < 8; ++y)
            {
                for (int x = 0; x < 8; ++x)
                {
                    int rank = (flip_board ? y : (7 - y));
                    int file = x;

                    int index = rank * 8 + file;
                    Cell &cell = board_cells[index];

                    wattron(board, COLOR_PAIR(cell.style));

                    for (int sx = 0; sx < BOARD_ASPECT_X; ++sx)
                    {
                        for (int sy = 0; sy < BOARD_ASPECT_Y; ++sy)
                        {
                            mvwaddch(board, BOARD_ASPECT_Y * y + sy, BOARD_ASPECT_X * x + sx, (sx == BOARD_ASPECT_X / 2 && sy == BOARD_ASPECT_Y / 2) ? cell.piece : ' ');
                        }
                    }

                    if (file == 0)
                        mvwaddch(board, BOARD_ASPECT_Y * y, BOARD_ASPECT_X * x, '1' + rank);

                    if (y == 7)
                        mvwaddch(board, BOARD_ASPECT_Y * (y + 1) - 1, BOARD_ASPECT_X * x, 'a' + file);
                    wattroff(board, COLOR_PAIR(cell.style));
                }
            }
            wnoutrefresh(board);
        }

        void get_material_string(std::string &white, std::string &black, int &eval)
        {
            static const int piece_values[] = {1, 3, 3, 5, 9, 0, -1, -3, -3, -5, -9, 0};

            white.clear();
            black.clear();
            eval = 0;

            int piece_counts[12] = {0};
            for (int c = 0; c < 2; ++c)
            {
                for (int pt = 0; pt < 6; ++pt)
                {
                    piece_counts[c * 6 + pt] = __builtin_popcountll(game->position.pieces[c][pt]);
                    eval -= piece_counts[c * 6 + pt] * piece_values[c * 6 + pt];
                }
            }

            for (int pt = 0; pt < 6; ++pt)
            {
                int diff = piece_counts[pt + 6] - piece_counts[pt];
                if (diff > 0)
                {
                    for (int i = 0; i < diff; ++i)
                        white += piece_names[pt];
                }
                else if (diff < 0)
                {
                    for (int i = 0; i < -diff; ++i)
                        black += piece_names[pt + 6];
                }
            }
        }

        void draw_sidebar()
        {
            WINDOW *sidebar = windows[W_SIDEBAR];

            werase(sidebar);

            int y = 0;

            std::string white_mat, black_mat;
            int eval = 0;
            get_material_string(white_mat, black_mat, eval);

            // Top material
            mvwprintw(sidebar, y++, 1, "%-18s %+3d", white_mat.c_str(), eval);

            // Frame top
            mvwhline(sidebar, y++, 1, 0, WINDOW_SIZES[W_SIDEBAR][1] - 2);
            wattron(sidebar, A_REVERSE);
            mvwprintw(sidebar, y++, WINDOW_SIZES[W_SIDEBAR][1] / 2 - 4, " chess ");
            mvwprintw(sidebar, y++, 1, "Press '?' for controls");
            wattroff(sidebar, A_REVERSE);
            mvwprintw(sidebar, y++, 1, "Turn: %s", game->position.turn() == chess::Color::WHITE ? "White" : "Black");

            y++; // Spacer

            int available_space = WINDOW_SIZES[W_SIDEBAR][0] - 10;
            int move_lines = (move_text.size() + 1) / 2;
            int first_move = std::max(0, move_lines - available_space);

            for (int i = first_move; i < move_lines; ++i)
            {
                std::string white_move = move_text[2 * i];
                std::string black_move = (2 * i + 1 < (int)move_text.size()) ? move_text[2 * i + 1] : " ";

                mvwprintw(sidebar, y++, 1, "%3d: %-7s %-7s", i + 1, white_move.c_str(), black_move.c_str());
            }
            y = 21;

            // Status message
            mvwprintw(sidebar, y++, 1, "%-22s", status.c_str());

            // Bottom material
            mvwhline(sidebar, y++, 1, 0, WINDOW_SIZES[W_SIDEBAR][1] - 2);
            mvwprintw(sidebar, y, 1, "%-18s %+3d", black_mat.c_str(), -eval);

            wnoutrefresh(sidebar);
        }

        void draw_help()
        {
            WINDOW *dialogue = windows[W_DIALOGUE];
            werase(dialogue);
            box(dialogue, 0, 0);

            int y = 0;

            mvwprintw(dialogue, y++, 1, " Help Menu ");
            wattron(dialogue, A_REVERSE);
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Command", "Key(s)");
            wattroff(dialogue, A_REVERSE);
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Movement", "h/j/k/l or arrow keys");
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Select", "space or enter");
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Cancel", "backspace/delete");
            y++;
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Flip board", "F");
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Undo/Redo", "u / r");
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Underpromotion", "Ctrl+Space");
            mvwprintw(dialogue, y++, 1, " %-14s %30s ", "Quit", "Q");
            y++;
            mvwprintw(dialogue, y, 1, " Press [Cancel] key to return ");

            wnoutrefresh(dialogue);
            doupdate();
        }

        void do_startup()
        {
            WINDOW *dialogue = windows[W_DIALOGUE];
            werase(dialogue);
            box(dialogue, 0, 0);

            mvwprintw(dialogue, 0, 1, " Chess ");

            wattron(dialogue, A_BOLD);
            mvwprintw(dialogue, 2, 2, "Choose Game Mode");
            wattroff(dialogue, A_BOLD);

            mvwprintw(dialogue, 3, 4, "[1] Player vs Player");
            mvwprintw(dialogue, 4, 4, "[2] Player vs Engine");
            mvwprintw(dialogue, 5, 4, "[3] Engine vs Engine");
            mvwprintw(dialogue, 6, 2, "Press number to continue...");

            wnoutrefresh(dialogue);
            doupdate();

            while (true)
            {
                int ch = getch();
                switch (ch)
                {
                case '1': // player vs player
                    white_engine = false;
                    black_engine = false;
                    return;
                case '2': // player vs engine
                    white_engine = true;
                    black_engine = true;
                    goto find_player_color;
                case '3': // engine vs engine
                    white_engine = true;
                    black_engine = true;
                    return;
                default:
                    beep();
                    break; // continue loop
                }
            }

        find_player_color:
            mvwprintw(dialogue, 2, 2, "Choose Game Mode");
            mvwprintw(dialogue, 6, 2, "                             "); // clear previous text

            wattron(dialogue, A_REVERSE);
            mvwprintw(dialogue, 4, 4, "[2] Player vs Engine");
            wattroff(dialogue, A_REVERSE);

            wattron(dialogue, A_BOLD);
            mvwprintw(dialogue, 8, 2, "Choose player color");
            wattroff(dialogue, A_BOLD);

            mvwprintw(dialogue, 9, 4, "[w] White");
            mvwprintw(dialogue, 10, 4, "[b] Black");
            mvwprintw(dialogue, 11, 2, "Press key to confirm...");

            wnoutrefresh(dialogue);
            doupdate();

            while (true)
            {
                int ch = getch();
                switch (ch)
                {
                case 'w':
                case 'W':
                    white_engine = false;
                    return;
                case 'b':
                case 'B':
                    black_engine = false;
                    return;
                default:
                    beep();
                    break; // continue loop
                }
            }
        }

        char get_underpromotion_choice()
        {
            WINDOW *dialogue = windows[W_DIALOGUE];
            wbkgd(dialogue, COLOR_PAIR(color_pairs::CYAN_GRAY));
            werase(dialogue);
            box(dialogue, 0, 0);

            mvwprintw(dialogue, 1, 2, "Underpromotion: Choose piece");
            mvwprintw(dialogue, 3, 4, "[q] Queen");
            mvwprintw(dialogue, 4, 4, "[r] Rook");
            mvwprintw(dialogue, 5, 4, "[b] Bishop");
            mvwprintw(dialogue, 6, 4, "[n] Knight");
            mvwprintw(dialogue, 8, 2, "Press key to confirm...");

            wnoutrefresh(dialogue);
            doupdate();

            while (true)
            {
                int ch = getch();
                switch (ch)
                {
                case 'q':
                case 'Q':
                    return static_cast<int>(chess::PieceType::QUEEN);
                case 'r':
                case 'R':
                    return static_cast<int>(chess::PieceType::ROOK);
                case 'b':
                case 'B':
                    return static_cast<int>(chess::PieceType::BISHOP);
                case 'n':
                case 'N':
                    return static_cast<int>(chess::PieceType::KNIGHT);
                default:
                    beep();
                }
            }
        }
    };

    static State state;

    // Initializes ncurses mode, sets up the screen
    void init(chess::Game *game)
    {
        if (!game)
        {
            game = new chess::Game();
            state.owns_game = true;
        }
        state.game = game;

        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        start_color();
        curs_set(0);
        use_default_colors();

        init_pair(COLOR_BLACK, COLOR_WHITE, COLOR_BLACK);

        // initialize colors (color defines the background name)
        {
            using namespace color_pairs;
            if (can_change_color())
            {
                // terminal supports custom colors
                init_color(NORMAL_LIGHT, 1000, 1000, 1000);
                init_color(NORMAL_DARK, 0, 0, 0);
                init_color(RED_LIGHT, 1000, 500, 500);
                init_color(RED_DARK, 500, 0, 0);
                init_color(GREEN_LIGHT, 500, 1000, 500);
                init_color(GREEN_DARK, 0, 500, 0);
                init_color(YELLOW_LIGHT, 1000, 1000, 500);
                init_color(YELLOW_DARK, 500, 500, 0);
                for (int tint = 0; tint < 5; ++tint)
                {
                    const short light = LIGHT | (tint << 1);
                    const short dark = DARK | (tint << 1);
                    init_pair(light, dark, light);
                    init_pair(dark, light, dark);
                }

                init_color(CYAN_GRAY, 155, 170, 175);
                init_pair(CYAN_GRAY, COLOR_WHITE, CYAN_GRAY);
            }
            else
            {
                // fallback: use predefined color pairs
                init_pair(NORMAL_LIGHT, COLOR_BLACK, COLOR_WHITE);
                init_pair(NORMAL_DARK, COLOR_WHITE, COLOR_BLACK);
                init_pair(RED_LIGHT, COLOR_BLACK, COLOR_RED);
                init_pair(RED_DARK, COLOR_WHITE, COLOR_RED);
                init_pair(GREEN_LIGHT, COLOR_BLACK, COLOR_GREEN);
                init_pair(GREEN_DARK, COLOR_WHITE, COLOR_GREEN);
                init_pair(YELLOW_LIGHT, COLOR_BLACK, COLOR_YELLOW);
                init_pair(YELLOW_DARK, COLOR_WHITE, COLOR_YELLOW);

                init_pair(CYAN_GRAY, COLOR_WHITE, COLOR_CYAN);
            }
        }

        state.init();
    }

    void cleanup()
    {
        state.running = false;
        if (state.owns_game)
            delete state.game;
        state.game = nullptr;
        endwin();
    }

    void update()
    {
        state.update();
    }

    void highlight_square(int row, int col, HighlightTint tint)
    {
        state.highlight_square(row * 8 + col, static_cast<short>(tint));
    }

    bool is_running()
    {
        return state.running;
    }

    static bool engine_turn();
    void display()
    {
        switch (state.mode)
        {
        case State::MD_STARTUP:
            state.do_startup();
            state.mode = State::MD_SELECT;
            break;
        case State::MD_SELECT:
            for (int sq = 0; sq < 64; ++sq)
            {
                state.highlight_square(sq, color_pairs::TINT_NORMAL);
            }
            if (state.check_square >= 0)
                state.highlight_square(state.check_square, color_pairs::TINT_RED);
            if (state.last_move_from >= 0)
                state.highlight_square(state.last_move_from, color_pairs::TINT_YELLOW);
            if (state.last_move_to >= 0)
                state.highlight_square(state.last_move_to, color_pairs::TINT_YELLOW);

            if (!((state.game->position.turn() == chess::Color::WHITE) ? state.white_engine : state.black_engine))
                state.highlight_square(state.cursor_square, color_pairs::TINT_GREEN);

            state.draw_sidebar();
            state.draw_board();
            break;
        case State::MD_SHOW_MOVES:
            for (int sq = 0; sq < 64; ++sq)
            {
                state.highlight_square(sq, color_pairs::TINT_NORMAL);
            }
            if (state.check_square >= 0)
                state.highlight_square(state.check_square, color_pairs::TINT_RED);
            if (state.selected_square >= 0)
                state.highlight_square(state.selected_square, color_pairs::TINT_GREEN);
            for (const auto &move : state.tile_moves[state.selected_square])
            {
                int to = chess::move::to(move);
                state.highlight_square(to, color_pairs::TINT_YELLOW);
            }
            state.highlight_square(state.cursor_square, color_pairs::TINT_GREEN);

            state.draw_sidebar();
            state.draw_board();
            break;

        case State::MD_GAME_OVER:
            for (int sq = 0; sq < 64; ++sq)
            {
                state.highlight_square(sq, color_pairs::TINT_NORMAL);
            }
            if (state.check_square >= 0)
                state.highlight_square(state.check_square, color_pairs::TINT_RED);
            if (state.last_move_from >= 0)
                state.highlight_square(state.last_move_from, color_pairs::TINT_YELLOW);
            if (state.last_move_to >= 0)
                state.highlight_square(state.last_move_to, color_pairs::TINT_YELLOW);
            state.draw_sidebar();
            state.draw_board();
            mvwprintw(state.windows[W_SIDEBAR], 3, 1, "Press Q to quit       ");
            wnoutrefresh(state.windows[W_SIDEBAR]);
            break;

        case State::MD_HELP:
            state.draw_help();
            break;

        default:
            break;
        }

        doupdate();
    }

    static void handle_input(int ch);
    void wait_for_input()
    {
        if (state.mode == State::MD_SELECT && engine_turn())
        {
            display();
            return;
        }

        while (true)
        {
            int ch = getch();
            if (ch == ERR)
                continue;

            if (ch == KEY_RESIZE)
            {
                state.check_resize();
                state.draw_board();
            }
            else
            {
                handle_input(ch);
                return;
            }
        }
    }

    // return true if the game is over
    static bool make_move(chess::Move m)
    {
        state.game->make_move(m);
        std::string move_text = chess::move::to_string(m);

        state.move_count = chess::get_moves(state.game->position, state.moves);
        bool check = state.game->position.king_checked(state.game->position.turn());

        state.check_square = -1;

        if (chess::move::is_promotion(m))
        {
            move_text += "pnbrqk"[chess::move::promo_piece_index(m)];
        }
        if (check)
        {
            state.check_square = __builtin_ctzll(state.game->position.pieces[(u8)state.game->position.turn()][(u8)chess::PieceType::KING]);
            move_text += (state.move_count > 0) ? "+" : "#";
        }

        state.move_text.push_back(move_text);
        state.last_move_from = chess::move::from(m);
        state.last_move_to = chess::move::to(m);

        state.update();

        return state.move_count > 0;
    }

    static bool engine_turn()
    {
        auto turn = state.game->position.turn();
        if ((turn == chess::Color::WHITE && state.white_engine) ||
            (turn == chess::Color::BLACK && state.black_engine))
        {
            state.status = "Engine thinking...";
            state.draw_sidebar();
            doupdate();
            int eval = 0;
            chess::Move engine_move = chess::engine::solve(*state.game, ENGINE_DEPTH, &eval);
            if (!engine_move)
            {
                state.status = "Engine error!";
                state.mode = State::MD_GAME_OVER;
                return false;
            }

            if (!make_move(engine_move))
            {
                state.mode = State::MD_GAME_OVER;
                bool check = state.game->position.king_checked(state.game->position.turn());
                state.status = check
                                   ? (state.game->position.turn() == chess::Color::WHITE
                                          ? "Checkmate! Black wins!"
                                          : "Checkmate! White wins!")
                                   : "Stalemate!";
                display();
                return false;
            }

            state.status.clear();
            state.update();
            return true;
        }

        return false;
    }

    static void handle_input(int ch)
    {
        Command cmd = get_command(ch);

        switch (cmd)
        {
        case CMD_QUIT:
            state.running = false;
            return;

        case CMD_UP:
        case CMD_DOWN:
            if (state.mode != State::MD_SELECT || state.mode != State::MD_SHOW_MOVES)
            {
                if ((cmd == CMD_UP) ^ state.flip_board)
                {
                    if (state.cursor_square < 56)
                        state.cursor_square += 8;
                }
                else
                {
                    if (state.cursor_square >= 8)
                        state.cursor_square -= 8;
                }
            }
            break;

        case CMD_LEFT:
            if (state.mode != State::MD_SELECT || state.mode != State::MD_SHOW_MOVES)
            {
                if (state.cursor_square % 8 != 0)
                    state.cursor_square -= 1;
            }
            break;

        case CMD_RIGHT:
            if (state.mode != State::MD_SELECT || state.mode != State::MD_SHOW_MOVES)
            {
                if (state.cursor_square % 8 != 7)
                    state.cursor_square += 1;
            }
            break;

        case CMD_HELP:
            if (state.mode != State::MD_HELP)
            {
                state.last_mode = state.mode;
                state.mode = State::MD_HELP;
            }
            break;

        case CMD_SELECT:
        case CMD_UNDERPROMOTION:
        {
            if (state.mode == State::MD_SELECT)
            {
                if (state.tile_moves[state.cursor_square].empty())
                    break;

                state.selected_square = state.cursor_square;
                state.mode = State::MD_SHOW_MOVES;
                break;
            }

            if (state.mode != State::MD_SHOW_MOVES)
                break;

            std::vector<chess::Move> matching;
            for (const auto &move : state.tile_moves[state.selected_square])
            {
                if (chess::move::to(move) == state.cursor_square)
                    matching.push_back(move);
            }

            if (matching.empty())
            {
                state.selected_square = -1;
                state.mode = State::MD_SELECT;
                break;
            }

            // default to the first move, which is the only one if no promotion
            chess::Move m = matching[0];

            if (chess::move::is_promotion(m))
            {
                // Promotion involved
                int promo_index = (cmd == CMD_UNDERPROMOTION)
                                      ? state.get_underpromotion_choice()
                                      : static_cast<int>(chess::PieceType::QUEEN); // default to queen

                for (const auto &move : matching)
                {
                    if (chess::move::promo_piece_index(move) == promo_index)
                    {
                        m = move;
                        break;
                    }
                }
            }
            state.selected_square = -1;
            state.mode = State::MD_SELECT;

            if (!make_move(m))
            {
                state.mode = State::MD_GAME_OVER;
                bool check = state.game->position.king_checked(state.game->position.turn());
                state.status = check
                                   ? (state.game->position.turn() == chess::Color::WHITE
                                          ? "Checkmate! Black wins!"
                                          : "Checkmate! White wins!")
                                   : "Stalemate!";
            }
            state.update();
        }
        break;
        case CMD_DESELECT:
            if (state.mode == State::MD_SHOW_MOVES)
            {
                state.mode = State::MD_SELECT;
                state.cursor_square = state.selected_square;
                state.selected_square = -1;
            }
            else if (state.mode == State::MD_HELP)
            {
                state.mode = state.last_mode;
            }
            break;

        case CMD_FLIP_BOARD:
            if (state.mode != State::MD_HELP)
            {
                state.flip_board = !state.flip_board;
            }
            break;
        case CMD_UNDO:
            // todo
            break;
        case CMD_REDO:
            // todo
            break;
        default:
            break;
        }
    }
} // namespace ui
