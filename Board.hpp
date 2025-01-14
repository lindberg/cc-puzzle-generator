#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <utility>
#include <vector>
#include <Eigen/Core>

#include "Piece.hpp"

using namespace std;

/*
   Stores a board, including placed pieces.
 */
class Board
{
    public:
        Board(string boardfilename = string("gamedata/board"));

        bool placePiece(Piece pc, int row, int col);
        void saveSolution(string outputDir);

        /*
        The state of the board is stored in the array barray
        as follows:

        Value:  Meaning:
         0-11    Pieces A-L
         12      border/forbidden
         13      empty

        Add a buffer on the right and lower edges so that
        we can place pieces without taking care of the boundaries.
        */
        enum {BORDER = 12, EMPTY = 13};

        static const int rows = 14;
        static const int cols = 20;
        static const int buffer = 8;
        Matrix<int8_t, rows+buffer, cols+buffer> barray;
        vector<vector<Piece>> pieces;

        /* Keep track of which pieces we have placed. */
        bool pieceplaced[12];

        bool isFull(); // true if all 12 pieces are placed
        Matrix<int8_t, 4, 6> freeSpaces(); // return free spaces
        int boardFileFormat[48];
        int piecesOnBoard;
};

ostream& operator<<(ostream& os, const Board& p);

#endif
