/*MIT License

Copyright (c) 2018 Minglei Wang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef fast_go_h
#define fast_go_h

#include <iostream>
#include <vector>
#include <stack>
#include <chrono>
#include <numeric>
#include <array>
#include <unordered_set>
#include <algorithm>
#include "config.h"

#define WHITE '2'
#define BLACK '1'
#define EMPTY '0'

#define NONE -1

#define NN WN*WN

class Position
{
private:
    std::string board;

public:
    int ko;

    Position()
    {
        this->board = "";
        this->ko = NONE;
    }
    
    Position(std::string board, int ko)
    {
        this->board = board;
        this->ko = ko;
    }

    static Position initial_state()
    {
        return Position(std::string(NN, EMPTY), NONE);
    }
    
    std::string get_board()
    {
        return this->board;
    }
    
    std::string str()
    {
        std::string s;
        for(int i=0; i<WN; i++)
        {
            s += this->board.substr(i*WN, WN);
            s += "\n";
        }
        return s;
    }
    
    //if stone fills into opponent's eye, it might be a suicide that will not change the board
    //if stone fills into its own eye, it might be suicide but wil change the board.
    //if stone has empty neighbour, it will not be removed so will change the board
    //so this function basically is to check if it fills into opponent's eye and did not kill any neighours
    bool board_not_changed(int fc, char color)
    {
        char possible_ko_color;
        bool is_ko = is_koish(board, fc, possible_ko_color);
        if(!is_ko)
            return false;
        if(possible_ko_color == color)
            return false;

        char opp_color = swap_colors(color);
        std::vector<int> opp_stones;
        std::vector<int> my_stones = {fc};
        
        neighbor_count = get_valid_neighbors(fc, neighbors);
        std::vector<int>::iterator it;
        for(int i=0; i<neighbor_count; i++)
        {
            int fn = neighbors[i];
            if (board[fn] == EMPTY)
                return false; //neight has liberty
            opp_stones.push_back(fn);
        }
        
        std::string new_board = place_stone(color, board, fc);
        //check all opponent neighbours to see if any of them would be captured meaning no liberty.
        for(it=opp_stones.begin(); it!=opp_stones.end(); ++it)
        {
            if(!has_liberty(new_board, *it))
                return false;
        }

        return true;
    }
    
    bool has_liberty(std::string board, int fc)
    {
        int frontier[NN];
        int frontier_count = 0;
        
        char color = board[fc];
        frontier[frontier_count++] = fc;
        
        while (frontier_count>0)
        {
            int current_fc = frontier[--frontier_count];
            
            board[current_fc] = '#';
            
            neighbor_count = get_valid_neighbors(current_fc, neighbors);
            for (int i = 0; i < neighbor_count; ++i)
            {
                int nfc = neighbors[i];
                if (board[nfc] == color)
                {
                    frontier[frontier_count++] = nfc;
                    board[nfc] = '#';
                }
                else if (board[nfc] == EMPTY)
                    return true;
            }
        }
        //cannot reach to an empty
        return false;
    }
    
    Position play_move(int fc, char color)
    {
        if (fc == this->ko || this->board[fc] != EMPTY)
        {
            std::cout << "board=" << this->board << std::endl;
            std::cout << "fc=" << fc << std::endl;
            throw std::invalid_argument( "illegal move" );
        }

        char possible_ko_color;
        bool is_ko = is_koish(board, fc, possible_ko_color);

        std::string new_board = place_stone(color, board, fc);
        char opp_color = swap_colors(color);
        std::vector<int> opp_stones;
        std::vector<int> my_stones = {fc};
        
        neighbor_count = get_valid_neighbors(fc, neighbors);
        std::vector<int>::iterator it;
        for(int i=0; i<neighbor_count; i++)
        {
            int fn = neighbors[i];
            if (new_board[fn] == color)
                my_stones.push_back(fn);
            else if(new_board[fn] == opp_color)
                opp_stones.push_back(fn);
        }
        
        std::vector<int> opp_captured;
        
        //check all opponent neighbours to see if they were captured.
        for(it=opp_stones.begin(); it!=opp_stones.end(); ++it)
        {
            std::vector<int> captured;
            new_board = maybe_capture_stones(new_board, *it, captured);
            
            //if captured more than 1, we don't care. we only need to keep the first capture which is ko if there is only one
            if(opp_captured.size()<=1)
                opp_captured.insert(opp_captured.end(), captured.begin(), captured.end());
        }

        //check all my neighbours to see if they were captured.
        for(it=my_stones.begin(); it!=my_stones.end(); ++it)
        {
            std::vector<int> captured;
            new_board = maybe_capture_stones(new_board, *it, captured);
        }

        int new_ko;
        if (is_ko && opp_captured.size() == 1 && possible_ko_color == opp_color)
            new_ko = opp_captured[0];
        else
            new_ko = NONE;

        return Position(new_board, new_ko);
    };
    
    float score()
    {
        if(this->board == std::string(NN, EMPTY))
            return -KOMI;
        
        std::string working_board = this->board;
        
        for(int i=0; i<working_board.length(); i++)
        {
            if(working_board[i]==EMPTY)
            {
                int fempty = i;
                
                std::vector<int> empties, borders;
                find_reached(board, fempty, empties, borders);
                
                char possible_border_color = board[*borders.begin()];
                if (std::all_of(borders.begin(), borders.end(), [=](int fb){return working_board[fb] == possible_border_color;}))
                    working_board = bulk_place_stones(possible_border_color, working_board, empties);
                else
                    // if an empty intersection reaches both white and black,
                    // then it belongs to neither player.
                    working_board = bulk_place_stones('?', working_board, empties);
            }
        }
        return ((int)std::count(working_board.begin(), working_board.end(), BLACK) \
             - (int)std::count(working_board.begin(), working_board.end(), WHITE)) \
             -  KOMI;
    }
    
    std::vector<int> possible_moves(char color)
    {
        std::array<int, NN+1> moves; // vector with all moves
        std::vector<int> disallowed_moved = not_allowed_actions(color);
        
        std::iota (std::begin(moves), std::end(moves), 0); // Fill with 0, 1, ..., 99.
        
        std::sort(moves.begin(), moves.end());
        std::sort(disallowed_moved.begin(), disallowed_moved.end());
        
        std::vector<int> difference;
        std::set_difference(
            moves.begin(), moves.end(),
            disallowed_moved.begin(), disallowed_moved.end(),
            std::back_inserter( difference )
        );

        return difference;
    }
    
    std::vector<int> not_allowed_actions(char color)
    {
        std::vector<int> moves;

        for(int move=0; move<board.length(); move++)
        {
            if(board[move]!=EMPTY || move==ko)
                moves.push_back(move);
            else
            {
                if(board_not_changed(move, color)) // not changing the board, then it is not legal
                {
                    moves.push_back(move);
                }
            }
        }

        return moves;
    }
    
    char swap_colors(const char color)
    {
        if (color == BLACK)
            return WHITE;
        else if (color == WHITE)
            return BLACK;
        else
            return color;
    }

    bool is_koish(int fc, char c)
    {
        char possible_ko_color;
        bool is_ko = is_koish(board, fc, possible_ko_color);
        if(is_ko && possible_ko_color == swap_colors(c))
            return true;
        else
            return false;
    }
    
private:
    int neighbors[4];
    int neighbor_count;
    inline int flatten(int x, int y)
    {
        return WN * x + y;
    }

    inline void unflatten(int fc, int& x, int& y)
    {
        x = (int)fc/WN;
        y = (int)fc%WN;
    }

    /*bool is_on_board(int x, int y)
    {
        return (x>=0 && x<WN && y>=0 && y<WN);
    }*/

    inline int get_valid_neighbors(int fc, int v[])
    {
        int x = (int)fc/WN;
        int y = (int)fc%WN;

        int count = 0;
        if(x-1>=0)
            v[count++] = WN * (x-1) + y;
        if(x+1<WN)
            v[count++] = WN * (x+1) + y;
        if(y-1>=0)
            v[count++] = WN * x + y-1;
        if(y+1<WN)
            v[count++] = WN * x + y+1;
        return count;
    }


    //chain is the area that fc belongs to
    //reached is the contour that surround the area
    void find_reached(std::string board, int fc, std::vector<int>& chain, std::vector<int>& reached)
    {
        char color = board[fc];
        int frontier[NN];
        int frontier_count = 0;
        
        frontier[frontier_count++] = fc;

        while (frontier_count>0)
        {
            int current_fc = frontier[--frontier_count]; //pop up stack

            board[current_fc] = '#'; //hash # is own's stones
            //chain.insert(current_fc);
            
            neighbor_count = get_valid_neighbors(current_fc, neighbors);
            for (int i = 0; i < neighbor_count; ++i)
            {
                int nfc = neighbors[i];
                if (board[nfc] == color )//&& chain.find(nfc)==chain.end())
                {
                    frontier[frontier_count++] = nfc;  //push into stack
                    board[nfc] = '#';
                }
                else if (board[nfc] != '*' && board[nfc] != '#')
                {
                    //reached.insert(nfc);
                    board[nfc] = '*';
                }
            }
        }
        
        chain.clear();
        reached.clear();
        for(int i=0; i<NN; i++)
        {
            if(board[i] == '#')
                chain.push_back(i);
            else if(board[i] == '*')
                reached.push_back(i);
        }
        return;
    }

    inline std::string place_stone(char color, std::string board, int fc)
    {
        board[fc] = color;
        return board;
    }

    std::string bulk_place_stones(char color, std::string board, std::vector<int>& stones)
    {
        std::for_each(stones.begin(), stones.end(), [&](int i){
            board[i] = color;
        });
        
        return board;
    }

    std::string maybe_capture_stones(std::string& board, int fc, std::vector<int>& chain)
    {
        std::vector<int> reached;
        
        find_reached(board, fc, chain, reached);
        
        //if all stones reached are not empty meaning no qi existing, remove the area from the board
        if( std::all_of(reached.begin(), reached.end(), [&](int fc){return board[fc] != EMPTY;}) )
            return bulk_place_stones(EMPTY, board, chain);
        else
	{
            chain.clear();
            return board;
        }
    }

    //if is ko, return true and surrounding color
    //if not, return false and EMPTY color
    bool is_koish(std::string& board, int fc, char& color)
    {
        if (board[fc] != EMPTY)
            return false;
        
        neighbor_count = get_valid_neighbors(fc, neighbors);

        color = EMPTY;
        
        int blackcount = 0, whitecount = 0;
        
        for(int i=0; i<neighbor_count; i++)
            if(board[neighbors[i]] == BLACK)
                blackcount++;
            else if(board[neighbors[i]] == WHITE)
                whitecount++;

        bool koish = false;
        if(blackcount==neighbor_count)
        {
            color = BLACK;
            koish = true;
        }
        else if(whitecount == neighbor_count)
        {
            color = WHITE;
            koish = true;
        }
        return koish;
    }
    
};


#endif /* fast_go_h */
