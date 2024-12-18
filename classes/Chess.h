#pragma once

#include "Game.h"
#include "ChessSquare.h"
#include "BitHolder.h"

const int pieceSize = 64;

enum ChessPiece {
    NoPiece = 0,
    Pawn = 1,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class Chess : public Game {
public:
    Chess();
    ~Chess();

    // Override functions for game setup and state management
    void setUpBoard() override;
    void stopGame() override;
    Player* checkForWinner() override;
    bool checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    // Chess-specific movement and action handling
    bool actionForEmptyHolder(BitHolder& holder) override;
    bool canBitMoveFrom(Bit& bit, BitHolder& src) override;
    bool canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;
    void bitMovedFromTo(Bit& bit, BitHolder& src, BitHolder& dst) override;

    // AI update logic
    void updateAI() override;

    // Board holder accessor
    BitHolder& getHolderAt(const int x, const int y) override { return _grid[y][x]; }

private:
    // Helper functions for setting up and managing pieces
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    void populateBoardFromFEN(const std::string& fen);
    void updatePieces(uint64_t& pieces, bool forWhite);
    std::vector<int> getRookMoves(int x, int y, bool isWhite);
    std::vector<int> getBishopMoves(int x, int y, bool isWhite);
    std::vector<int> getKnightMoves(int x, int y, bool isWhite);
    std::vector<int> getPawnMoves(int x, int y, bool isWhite);
    std::vector<int> getKingMoves(int x, int y, bool isWhite);

    const char bitToPieceNotation(int row, int column) const;

    // 8x8 chessboard grid
    ChessSquare _grid[8][8];

    // Bitboards for white and black pieces
    uint64_t wPieces = 0; // Bitboard for white pieces
    uint64_t bPieces = 0; // Bitboard for black pieces
};
