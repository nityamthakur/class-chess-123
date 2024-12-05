#include "Chess.h"
#include <vector>

const int AI_PLAYER = 1;
const int HUMAN_PLAYER = -1;

Chess::Chess()
{
}

Chess::~Chess()
{
}

//
// make a chess piece for the player
//
Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    // depending on playerNumber load the "x.png" or the "o.png" graphic
    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("chess/") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard() {
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    // Initialize the grid with squares
    for (int y = 0; y < _gameOptions.rowY; ++y) {
        for (int x = 0; x < _gameOptions.rowX; ++x) {
            ImVec2 position((float)(pieceSize * x + pieceSize), (float)(pieceSize * (7 - y) + pieceSize));
            _grid[y][x].initHolder(position, "boardsquare.png", x, y);
            _grid[y][x].setGameTag(0); // Empty square initially
        }
    }

    // Reset bitboards
    wPieces = 0;
    bPieces = 0;

    // Use FEN to set up the board
    std::string initialFEN = "RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr";
    populateBoardFromFEN(initialFEN);

    startGame();
}

void Chess::populateBoardFromFEN(const std::string& fen) {
    static const std::unordered_map<char, ChessPiece> pieceMap = {
        {'p', Pawn}, {'r', Rook}, {'n', Knight}, {'b', Bishop}, {'q', Queen}, {'k', King}
    };

    int index = 0; // Linear index for the board (0-63)
    for (char c : fen) {
        if (c == '/') {
            continue; // Move to the next row in FEN
        } else if (isdigit(c)) {
            index += c - '0'; // Skip empty squares
        } else {
            int row = index / 8;
            int col = index % 8;
            bool isWhite = isupper(c);
            ChessPiece piece = pieceMap.at(tolower(c));

            Bit* bit = PieceForPlayer(isWhite ? 0 : 1, piece);
            bit->setPosition(_grid[row][col].getPosition());
            bit->setParent(&_grid[row][col]);
            _grid[row][col].setBit(bit);

            // Set game tag for tracking
            int gameTag = piece + (isWhite ? 0 : 128);
            bit->setGameTag(gameTag);
            _grid[row][col].setGameTag(gameTag);

            // Update the bitboard for white or black pieces
            uint64_t bitPos = 1ULL << index;
            if (isWhite) {
                wPieces |= bitPos;
            } else {
                bPieces |= bitPos;
            }
            ++index;
        }
    }
}

//
// about the only thing we need to actually fill out for tic-tac-toe
//
bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    return true;
}
bool Chess::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);

    if (!srcSquare || !dstSquare) return false; // Ensure proper casting

    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();

    int player = getCurrentPlayer()->playerNumber();
    int gameTag = bit.gameTag();
    bool isWhite = gameTag < 128;
   // uint64_t playerPieces = isWhite ? wPieces : bPieces;

    // Ensure it's the player's turn and piece belongs to them
    if ((player == 0 && !isWhite) || (player == 1 && isWhite)) return false;

    // Check if the piece can move to the destination
    std::vector<int> validMoves;
    switch (gameTag % 128) {
        case Rook: validMoves = getRookMoves(srcX, srcY, isWhite); break;
        case Bishop: validMoves = getBishopMoves(srcX, srcY, isWhite); break;
        case Queen:
            validMoves = getRookMoves(srcX, srcY, isWhite);
            {
                auto bishopMoves = getBishopMoves(srcX, srcY, isWhite);
                validMoves.insert(validMoves.end(), bishopMoves.begin(), bishopMoves.end());
            }
            break;
        case Pawn: validMoves = getPawnMoves(srcX, srcY, isWhite); break;
        case Knight: validMoves = getKnightMoves(srcX, srcY, isWhite); break;
        case King: validMoves = getKingMoves(srcX, srcY, isWhite); break;
        default: return false;
    }

    int target = dstY * 8 + dstX;
    return std::find(validMoves.begin(), validMoves.end(), target) != validMoves.end();
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    int pieceTag = (bit.gameTag() > 128) ? bit.gameTag() - 128 : bit.gameTag();  // Adjust for black pieces
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);

    int x = srcSquare->getColumn();
    int y = srcSquare->getRow();
    int total = (8 * y) + x;

    int x2 = dstSquare->getColumn();
    int y2 = dstSquare->getRow();
    int total2 = (8 * y2) + x2;

    // Update grid and bitboard for the source and destination
    _grid[y][x].setGameTag(0);  // Clear the source square
    _grid[y2][x2].setGameTag(bit.gameTag());  // Set the destination square

    // Update bitboards
    updatePieces(wPieces, 0);
    updatePieces(bPieces, 1);

    // Additional functionality can be added here if needed

    endTurn();  // Signal end of turn
}

void Chess::updatePieces(uint64_t& pieces, bool forWhite) {
    pieces = 0; // Clear current bitboard
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            int gameTag = _grid[y][x].gameTag();
            bool isPieceWhite = (gameTag > 0 && gameTag < 128);
            if ((forWhite && isPieceWhite) || (!forWhite && gameTag > 128)) {
                uint64_t bitPosition = 1ULL << (y * 8 + x);
                pieces |= bitPosition; // Add piece position to bitboard
            }
        }
    }
}

std::vector<int> Chess::getRookMoves(int x, int y, bool isWhite) {
    std::vector<int> moves;
    int dx[] = {1, -1, 0, 0}; // Directions for rook (horizontal and vertical)
    int dy[] = {0, 0, 1, -1};

    for (int dir = 0; dir < 4; ++dir) {
        int nx = x, ny = y;
        while (true) {
            nx += dx[dir];
            ny += dy[dir];

            if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) break; // Out of bounds

            Bit* bit = _grid[ny][nx].bit();
            if (bit) {
                if ((bit->gameTag() < 128) != isWhite) moves.push_back(ny * 8 + nx); // Enemy piece
                break; // Blocked
            }
            moves.push_back(ny * 8 + nx); // Empty square
        }
    }
    return moves;
}
std::vector<int> Chess::getBishopMoves(int x, int y, bool isWhite) {
    std::vector<int> moves;
    int dx[] = {1, 1, -1, -1}; // Directions for bishop (diagonals)
    int dy[] = {1, -1, 1, -1};

    for (int dir = 0; dir < 4; ++dir) {
        int nx = x, ny = y;
        while (true) {
            nx += dx[dir];
            ny += dy[dir];

            if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) break; // Out of bounds

            Bit* bit = _grid[ny][nx].bit();
            if (bit) {
                if ((bit->gameTag() < 128) != isWhite) moves.push_back(ny * 8 + nx); // Enemy piece
                break; // Blocked
            }
            moves.push_back(ny * 8 + nx); // Empty square
        }
    }
    return moves;
}
std::vector<int> Chess::getKnightMoves(int x, int y, bool isWhite) {
    std::vector<int> moves;
    int dx[] = {2, 2, -2, -2, 1, -1, 1, -1}; // Knight jumps
    int dy[] = {1, -1, 1, -1, 2, 2, -2, -2};

    for (int i = 0; i < 8; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) continue; // Out of bounds

        Bit* bit = _grid[ny][nx].bit();
        if (!bit || (bit->gameTag() < 128) != isWhite) moves.push_back(ny * 8 + nx); // Empty or enemy
    }
    return moves;
}
std::vector<int> Chess::getPawnMoves(int x, int y, bool isWhite) {
    std::vector<int> moves;
    int forward = isWhite ? 1 : -1; // White moves down (+1), Black moves up (-1)

    // Single forward move
    if (y + forward >= 0 && y + forward < 8 && !_grid[y + forward][x].bit()) {
        moves.push_back((y + forward) * 8 + x);
    }

    // Double forward move (only on initial rank)
    if ((isWhite && y == 1) || (!isWhite && y == 6)) {
        if (!_grid[y + forward][x].bit() && !_grid[y + 2 * forward][x].bit()) {
            moves.push_back((y + 2 * forward) * 8 + x);
        }
    }

    // Diagonal captures
    for (int dx : {-1, 1}) { // Check diagonals
        int nx = x + dx;
        int ny = y + forward;
        if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            Bit* bit = _grid[ny][nx].bit();
            if (bit && (bit->gameTag() < 128) != isWhite) { // Enemy piece
                moves.push_back(ny * 8 + nx);
            }
        }
    }

    return moves;
}
std::vector<int> Chess::getKingMoves(int x, int y, bool isWhite) {
    std::vector<int> moves;
    int dx[] = {1, -1, 0, 0, 1, -1, 1, -1}; // Adjacent squares
    int dy[] = {0, 0, 1, -1, 1, -1, -1, 1};

    for (int i = 0; i < 8; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) continue; // Out of bounds

        Bit* bit = _grid[ny][nx].bit();
        if (!bit || (bit->gameTag() < 128) != isWhite) moves.push_back(ny * 8 + nx); // Empty or enemy
    }
    return moves;
}
//
// free all the memory used by the game on the heap
//
void Chess::stopGame()
{
}

Player* Chess::checkForWinner()
{
    // check to see if either player has won
    return nullptr;
}

bool Chess::checkForDraw()
{
    // check to see if the board is full
    return false;
}

//
// add a helper to Square so it returns out FEN chess notation in the form p for white pawn, K for black king, etc.
// this version is used from the top level board to record moves
//
const char Chess::bitToPieceNotation(int row, int column) const {
    if (row < 0 || row >= 8 || column < 0 || column >= 8) {
        return '0';
    }

    const char* wpieces = { "?PNBRQK" };
    const char* bpieces = { "?pnbrqk" };
    unsigned char notation = '0';
    Bit* bit = _grid[row][column].bit();
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag() & 127];
    } else {
        notation = '0';
    }
    return notation;
}

//
// state strings
//
std::string Chess::initialStateString()
{
    return stateString();
}

//
// this still needs to be tied into imguis init and shutdown
// we will read the state string and store it in each turn object
//
std::string Chess::stateString()
{
    std::string s;
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            s += bitToPieceNotation(y, x);
        }
    }
    return s;
}

//
// this still needs to be tied into imguis init and shutdown
// when the program starts it will load the current game from the imgui ini file and set the game state to the last saved state
//
void Chess::setStateString(const std::string &s)
{
    for (int y = 0; y < _gameOptions.rowY; y++) {
        for (int x = 0; x < _gameOptions.rowX; x++) {
            int index = y * _gameOptions.rowX + x;
            int playerNumber = s[index] - '0';
            if (playerNumber) {
                _grid[y][x].setBit(PieceForPlayer(playerNumber - 1, Pawn));
            } else {
                _grid[y][x].setBit(nullptr);
            }
        }
    }
}


//
// this is the function that will be called by the AI
//
void Chess::updateAI() 
{
}
