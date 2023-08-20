#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "Board.hpp"
#include "colors.hpp"
#include "readstuff.hpp"

using namespace std;

const int Board::rows;
const int Board::cols;
const int Board::buffer;

Board::Board(string boardfilename)
{
    Board::pieces = readpieces();
    barray.setConstant(BORDER);

    ifstream boardfile(boardfilename.c_str());

    // Read shape of the board from file
    int row = 0;
    char linebuf[100];
    while(boardfile.getline(linebuf, 100) && row < 14)
    {
        string line(linebuf);
        for(int col = 0; col < min(20, (int)line.length()); col++)
        {
            if (line.at(col) == 'X')
                barray(row, col) = BORDER;
            else
                barray(row, col) = EMPTY;
        }
        row++;
    }

    for (int ii = 0; ii < 12; ii++)
        pieceplaced[ii] = false;

    for (int ii = 0; ii < 48; ii++)
        boardFileFormat[ii] = -1;
    
    piecesOnBoard = 0;
}

void Board::saveSolution(string outputDir) {
    std::ofstream outFile(outputDir); // Change the file name as needed

    if (outFile.is_open()) {
        for (int i = 0; i < 48; ++i) {
            outFile << boardFileFormat[i];
            if ((i + 1) % 4 == 0 && i != 47) {
                outFile << "\n";
            } else {
                outFile << " ";
            }
        }
        outFile.close();
        std::cout << "Data written to the file successfully." << std::endl;
    } else {
        std::cerr << "Unable to open the file." << std::endl;
    }
}

bool Board::placePiece(Piece pc, int crow, int ccol)
{
    /*
     * crow, ccol: coarse row, col. Corresponds to the dots
     * in gamedata/board.
     *
     * If the piece Piece can be placed at (row,col),
     * change the state of the board and return true.
     *
     * Otherwise, the board remains unchanged and false
     * is returned.
     *
     * */

    /* Test if the piece can be placed. */
    if (pieceplaced[pc.number])
        return false;

    int m = pc.shape.rows();
    int n = pc.shape.cols();

    for (int ii = 0; ii < m; ii++)
    {
        for (int jj = 0; jj <n; jj++)
        {
            int row = 3*crow+ii;
            int col = 3*ccol+jj;
            if (pc.shape(ii,jj) == 1 && barray(row,col) != Board::EMPTY)
                return false; // collision found.
        }
    }

    // Piece fits, place it
    for (int ii = 0; ii < m; ii++)
    {
        for (int jj = 0; jj <n; jj++)
        {
            int row = 3*crow+ii;
            int col = 3*ccol+jj;
            if (pc.shape(ii,jj) == 1)
                barray(row,col) = pc.number;
        }
    }
    pieceplaced[pc.number] = true;

    // Also store board in input file format
    for (size_t i = 0; i < pieces[pc.number].size(); i++) {
        if (pieces[pc.number][i] == pc) {
            boardFileFormat[piecesOnBoard*4] = pc.number;
            boardFileFormat[piecesOnBoard*4+1] = i; // piecevar
            boardFileFormat[piecesOnBoard*4+2] = crow;
            boardFileFormat[piecesOnBoard*4+3] = ccol;
        }
    }

    piecesOnBoard++;

    return true;
}

bool Board::isFull()
{
    bool full = true;
    for (int ii = 0; ii < 12; ii++)
        full = full && pieceplaced[ii];

    return full;
}

Matrix<int8_t, 4, 6> Board::freeSpaces()
{
    Matrix<int8_t, 4, 6> freesp;

    for (int crow = 0; crow < 4; crow++)
        for (int ccol = 0; ccol < 6; ccol++)
            if (barray(2+3*crow, 2+3*ccol) == EMPTY)
                freesp(crow,ccol) = 0;
            else
                freesp(crow,ccol) = 1;

    return freesp;
}

ostream& operator<<(ostream& os, const Board& board)
{
    for(int row = 0; row < board.rows; row++)
    {
        for(int col = 0; col < board.cols; col++)
        {
            uint8_t thissq = board.barray(row,col);
            switch (thissq)
            {
                case Board::EMPTY:
                    os << "  "; break;
                case Board::BORDER:
                    os << "XX"; break;
                default:
                    if (thissq <= 11)
                    {
                        char colbuf[20];
                        sprintf(colbuf, "\033[48;5;%im", COLORS[thissq]);
                        string ansicolor(colbuf);
                        os << ansicolor << "  " << "\033[0m";
                    }
            }
        }
        os << std::endl;
    }

    return os;
}

