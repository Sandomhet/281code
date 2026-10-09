// Project Identifier: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD

#ifndef TREASUREHUNT_HPP
#define TREASUREHUNT_HPP

#include <deque>
#include <ostream>
#include <string>
#include <vector>

// Which end of the deque is "next": top for a stack, front for a queue.
enum class Container {
    kStack,
    kQueue,
};
enum class PathMode {
    kNone,
    kMap,
    kList,
};


struct Options {
    Container captain = Container::kStack;
    Container firstMate = Container::kQueue;
    std::string huntOrder = "NESW";
    bool verbose = false;
    bool stats = false;
    PathMode pathMode = PathMode::kNone;
};


struct Coordinate {
    int row = 0;
    int col = 0;

    bool operator==(const Coordinate &other) const { return row == other.row && col == other.col; }
    bool operator!=(const Coordinate &other) const { return !(*this == other); }

    friend std::ostream &operator<<(std::ostream &os, const Coordinate &cell) {
        return os << cell.row << ',' << cell.col;
    }
};


class TreasureHunt {
public:
    explicit TreasureHunt(const Options &opts) : options(opts) {}

    void readInput();
    void hunt();
    void printResults();

private:
    Options options;
    int size = 0;
    std::vector<std::vector<char>> grid;      // terrain
    std::vector<std::vector<char>> cameFrom;  // direction moved to discover a cell; 0 = undiscovered
    Coordinate start;
    Coordinate treasure;
    bool found = false;
    int waterInvestigated = 0;
    int landInvestigated = 0;
    int wentAshore = 0;

    char &terrainAt(Coordinate cell) { return grid[cell.row][cell.col]; }
    char &cameFromAt(Coordinate cell) { return cameFrom[cell.row][cell.col]; }

    // Sets `next` to the neighbor of `cell` in direction `dir`; false if off the grid.
    bool neighbor(Coordinate cell, char dir, Coordinate &next) const;
    // Inverse of neighbor(): the cell `cell` was discovered from.
    Coordinate previous(Coordinate cell);

    static Coordinate take(std::deque<Coordinate> &container, Container type);
    void search(Coordinate ashore);
};

#endif
