#include <algorithm>
#include <iostream>
#include <set>
#include <stack>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <limits>

#include "Board.hpp"
#include "Piece.hpp"
#include "readstuff.hpp"

using namespace std;

static const bool GENERATE_ALL_SOLUTIONS = false;

vector<vector<Piece> > pieces;

inline
bool matchpattern(Board& board, Matrix<int8_t, Dynamic, Dynamic> patt)
{
    /* Match a pattern patt on the Board board.
     *
     * element (1,1) in patt is its 'center'
     *
     * Values:
     * 0:  empty
     * 1:  occupied
     * -1: don't check
     */
    int mf = Board::rows;
    int nf = Board::cols;
    int mp = patt.rows();
    int np = patt.cols();
    for (int row = 0; row < mf; row++)
        for (int col = 0; col < nf; col++)
        {
            bool match = true;
            for (int rp = 0; rp < mp; rp++)
                for (int cp = 0; cp < np; cp++)
                {
                    if (patt(rp,cp) == -1) continue; // don't check

                    int8_t thissq_board = 0;
                    if (row+rp-1 < 0 || col+cp-1 < 0)
                        thissq_board = Board::BORDER;
                    else
                        thissq_board = board.barray(row+rp-1,col+cp-1);

                    int8_t thissq_patt = patt(rp,cp);
                    bool bothempty =
                        thissq_board == Board::EMPTY && thissq_patt == 0;
                    bool bothfilled =
                        thissq_board != Board::EMPTY && thissq_patt == 1;
                    bool squarematch = bothempty || bothfilled;

                    if (!squarematch)
                        match = false;
                }

            if (match) return true;
        }
    return false;
}

inline
bool hasisolated3x3(Matrix<int8_t, 4, 6>& freesp)
{
    int mc = freesp.rows();
    int nc = freesp.cols();
    for (int crow = 0; crow < mc; crow++)
        for (int ccol = 0; ccol < nc; ccol++)
            if (freesp(crow,ccol) == 0)
            {
                // Find at least one free neighbor (up down left right)
                int neighbors = 0;

                if (crow != 0 && freesp(crow-1, ccol) == 0) // up
                    neighbors++;

                if (crow != mc-1 && freesp(crow+1, ccol) == 0) // down
                    neighbors++;

                if (ccol != 0 && freesp(crow, ccol-1) == 0) // left
                    neighbors++;

                if (ccol != nc-1 && freesp(crow, ccol+1) == 0) // right
                    neighbors++;

                if (neighbors == 0) // found an isolated coarse block
                    return true;
            }

    return false;
}

pair<int,int> firstfreespace(Matrix<int8_t, 4,6> &freesp)
{
    for (int crow = 0; crow < 4; crow++)
        for (int ccol = 0; ccol < 6; ccol++)
            if (freesp(crow,ccol) == 0)
                return pair<int,int>(crow,ccol);

    return pair<int,int>(-1,-1);
}

void solve(vector<Board>& solutions, stack<Board>& boardstack,
        int& moves, int maxmoves, int maxsol)
{
    if (GENERATE_ALL_SOLUTIONS) {
        maxmoves = numeric_limits<int>::max();
        maxsol = numeric_limits<int>::max();
    }
    while(!boardstack.empty())
    {
        Board board = boardstack.top();
        boardstack.pop();

        /* See if this is a solution */
        if (board.isFull())
        {
            solutions.insert(solutions.end(), board);
            cout << "Solutions found: " << solutions.size() << endl;

            if (GENERATE_ALL_SOLUTIONS) {
                board.saveSolution("solutions/" + to_string(solutions.size()));
            }
        }

        /* Are we done? */
        if (solutions.size() >= maxsol) return;
        if (moves >= maxmoves) return;

        /* Check free spaces. 0 = free, 1 = taken. */
        Matrix<int8_t, 4, 6> freesp = board.freeSpaces();

        /* Check for isolated coarse blocks. */
        if (hasisolated3x3(freesp)) continue;

        /* Check for 1x1, 1x2 and 2x1 isolated blocks in the fine grid,
         * i.e. look for the following patterns:
         *
         *  1      2
         * XXXXXX ??XXXX?? and the transpose of 2.
         * XX  XX XX    XX
         * XXXXXX ??XXXX??
         */
        Matrix<int8_t, 3,3> patt1;
        Matrix<int8_t, 4,3> patt2;
        Matrix<int8_t, 3,4> patt3;
        patt1 << 1,1,1, 1,0,1, 1,1,1;
        patt2 << -1,1,-1, 1,0,1, 1,0,1, -1,1,-1;
        patt3 << -1,1,1,-1, 1,0,0,1, -1,1,1,-1;
        //if (matchpattern(board, patt1)) continue; // is 50% slower ?!
        if (matchpattern(board, patt2)) continue;
        if (matchpattern(board, patt3)) continue;

        /* Find the first free space, then try all possible pieces
         * and orientations. Recurse on all feasible moves. */
        pair<int,int> placeat = firstfreespace(freesp);
        int crow = placeat.first;
        int ccol = placeat.second;

        for (int ii = 0; ii < 12; ii++)
        {
            if (board.pieceplaced[ii]) // don't place a piece twice
                continue;

            // for all piece orientations
            for (int jj = 0; jj < pieces[ii].size(); jj++)
            {
                // try to place at the first empty space
                Board tryboard = board;
                bool pieceok =
                    tryboard.placePiece(pieces[ii][jj], crow, ccol);

                if (pieceok) // recurse
                {
                    moves++;
                    boardstack.push(tryboard);
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        cout << endl;
        cout << "Please specify a board file to solve." << endl;
        cout << "Usage: solve <filename>" << endl;
        return 0;
    }

    int maxmoves = 100000; // Default maximum number of moves
    for (int i = 2; i < argc; i++)
    {
        if (string(argv[i]) == "-m")
        {
            if (i + 1 < argc)
            {
                maxmoves = atoi(argv[i + 1]);
                break;
            }
        }
    }

    pieces = readpieces();

    /* Read board from file */
    string filename(argv[1]);
    Board startboard;

    try {
        startboard = readboard(filename);
    } catch (const std::runtime_error& e) {
        cerr << "Error: " << e.what() << std::endl;
        return 0;
    }

    cout << endl << filename << endl << endl;
    cout << startboard << endl;

    /* Solve */
    int moves = 0;
    int maxsol = 3; // quit after this number of solutions

    stack<Board> boardstack;
    boardstack.push(startboard);
    vector<Board> solutions;
    solve(solutions, boardstack, moves, maxmoves, maxsol);

    cout << "Found " << solutions.size() << " solutions using " << moves << " moves." << endl;

    if (solutions.size() > 0)
    {
        cout << endl;
        cout << "Sample solution:" << endl;
        cout << *solutions.begin() << endl;
    }
}

