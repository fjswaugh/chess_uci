#include "chess/chess.h"

#include <thread>
#include <chrono>
#include <sstream>

using namespace std::literals;

constexpr auto initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void print(const Chess::Position& position)
{
    std::cout << (position.active_player == Chess::Player::white ? "White" : "Black")
              << " to move\n";
    for (auto row = 8; row-- > 0;) {
        for (auto col = 0; col < 8; ++col) {
            char ch = to_char(position.mailbox[Chess::Location(col, row)]);
            std::cout << (ch == ' ' ? '.' : ch) << ' ';
        }
        std::cout << '\n';
    }
}

void go(Chess::Io& io, const Chess::Position& p, Chess::Transposition_table& tt)
{
    const auto f = [&] { Chess::recommend_move(io, p, tt); };

    io.go();

    std::thread t(f);
    t.detach();
}

void stop(Chess::Io& io)
{
    io.stop();
    
    // Wait for the thread to end
    // I'm sure there is a better way of doing this...
    std::this_thread::sleep_for(50ms);
}

Chess::Position parse_position(std::istringstream& input)
{
    Chess::Position position;

    std::string command;
    while (input >> command) {
        if (command == "fen") {
            input >> position;
        } else if (command == "startpos") {
            position = Chess::Position::from_fen(initial_fen);
        } else if (command == "moves") {
            std::string move_str;
            while (input >> move_str) {
                const auto from = Chess::Location(&move_str[0]);
                const auto to = Chess::Location(&move_str[2]);
                const auto piece = move_str.length() == 5
                                       ? Chess::to_piece(Chess::to_square(move_str[4]))
                                       : Chess::Piece::empty;
                const auto move = Chess::deduce_move_from_coordinates(position, from, to, piece);

                position = Chess::apply(move, position);
            }
        }
    }

    return position;
}

void uci_communication()
{
    Chess::Position position;
    Chess::Transposition_table tt;

    Chess::Io io;
    io.report_score = [](Chess::i16 score) {
        std::cout << "info score cp " << score << '\n';
    };
    io.report_best_move = [&position](Chess::Move move) {
        std::cout << "bestmove " << to_string(move.from()) << to_string(move.to());
        if (is_promotion(move.info())) {
            const auto promotion_piece = Chess::promotion_piece(move.info());
            if (promotion_piece == Chess::Piece::rook)   std::cout << 'r';
            if (promotion_piece == Chess::Piece::knight) std::cout << 'n';
            if (promotion_piece == Chess::Piece::bishop) std::cout << 'b';
            if (promotion_piece == Chess::Piece::queen)  std::cout << 'q';
        }
        std::cout << std::endl;

        position = apply(move, position);
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream ss{line};

        std::string first_word;
        ss >> first_word;

        if (first_word == "uci") {
            std::cout << "id name Chess\n";
            std::cout << "id author Fergus Waugh\n";
            std::cout << "uciok\n";
        } else if (first_word == "setoption") {
            // Don't care about this right now
        } else if (first_word == "isready") {
            std::cout << "readyok\n";
        } else if (first_word == "ucinewgame") {
            // Don't need to do anything here yet
        } else if (first_word == "position") {
            position = parse_position(ss);
        } else if (first_word == "go") {
            go(io, position, tt);
        } else if (first_word == "stop") {
            stop(io);
        } else if (first_word == "print") {
            print(position);
        }
    }
}

int main()
{
    uci_communication();
}

