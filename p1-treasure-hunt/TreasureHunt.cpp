// Project Identifier: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD

#include "TreasureHunt.hpp"

#include <cstdlib>
#include <iostream>

using namespace std;


bool TreasureHunt::neighbor(Coordinate cell, char dir, Coordinate &next) const {
    next = cell;
    switch (dir) {
    case 'N':
        if (cell.row == 0) return false;
        --next.row;
        return true;
    case 'S':
        if (cell.row + 1 == size) return false;
        ++next.row;
        return true;
    case 'W':
        if (cell.col == 0) return false;
        --next.col;
        return true;
    default:  // 'E'
        if (cell.col + 1 == size) return false;
        ++next.col;
        return true;
    }
}


Coordinate TreasureHunt::previous(Coordinate cell) {
    switch (cameFromAt(cell)) {
    case 'N':
        ++cell.row;
        break;
    case 'S':
        --cell.row;
        break;
    case 'W':
        ++cell.col;
        break;
    default:  // 'E'
        --cell.col;
        break;
    }
    return cell;
}


void TreasureHunt::readInput() {
    string line;
    while (getline(cin, line) && !line.empty() && line[0] == '#') { } // skip comment lines
    char format = line[0];
    cin >> size;

    grid.assign(size, vector<char>(size, '.'));
    cameFrom.assign(size, vector<char>(size, '\0'));
    bool haveStart = false;
    bool haveTreasure = false;

    auto place = [&](Coordinate cell, char terrain) {
        switch (terrain) {
        case '@':
            start = cell;
            haveStart = true;
            break;
        case '$':
            treasure = cell;
            haveTreasure = true;
            break;
        case '.':
        case 'o':
        case '#':
            break;
        default:
            cerr << "Invalid terrain type\n";
            exit(1);
        }
        terrainAt(cell) = terrain;
    };

    if (format == 'M') {
        for (int r = 0; r < size; ++r) {
            cin >> line;
            for (int c = 0; c < size; ++c)
                place({ r, c }, line[c]);
        }
    } else {
        int r = 0, c = 0;
        char terrain = '\0';
        while (cin >> r >> c >> terrain) {
            if (r < 0 || r >= size || c < 0 || c >= size) {
                cerr << "Invalid coordinates in list mode input\n";
                exit(1);
            }
            place({ r, c }, terrain);
        }
    }

    if (!haveStart) {
        cerr << "Map does not have a start location\n";
        exit(1);
    }
    if (!haveTreasure) {
        cerr << "Map does not have treasure\n";
        exit(1);
    }
}


Coordinate TreasureHunt::take(deque<Coordinate> &container, Container type) {
    Coordinate cell;
    if (type == Container::kStack) {
        cell = container.back();
        container.pop_back();
    } else {
        cell = container.front();
        container.pop_front();
    }
    return cell;
}


void TreasureHunt::search(Coordinate ashore) {
    ++wentAshore;
    if (options.verbose) {
        cout << "Went ashore at: " << ashore << '\n';
    }

    // Reaching the treasure counts it as investigated (matches reference output).
    if (ashore == treasure) {
        found = true;
        ++landInvestigated;
    } else {
        deque<Coordinate> searchContainer{ ashore };
        while (!found && !searchContainer.empty()) {
            Coordinate current = take(searchContainer, options.firstMate);
            ++landInvestigated;
            for (char dir : options.huntOrder) {
                Coordinate next;
                if (!neighbor(current, dir, next) || cameFromAt(next) != '\0')
                    continue;
                char terrain = terrainAt(next);
                if (terrain != 'o' && terrain != '$')
                    continue;
                cameFromAt(next) = dir;
                searchContainer.push_back(next);
                if (terrain == '$') {
                    found = true;
                    ++landInvestigated;
                    break;
                }
            }
        }
    }

    if (options.verbose) {
        if (found) {
            cout << "Searching island... party found treasure at " << treasure << ".\n";
        } else {
            cout << "Searching island... party returned with no treasure.\n";
        }
    }
}


void TreasureHunt::hunt() {
    if (options.verbose) {
        cout << "Treasure hunt started at: " << start << '\n';
    }

    // Mark start as discovered; its direction is never read during backtracking.
    cameFromAt(start) = 'X';
    deque<Coordinate> sailContainer{ start };
    while (!found && !sailContainer.empty()) {
        Coordinate current = take(sailContainer, options.captain);
        ++waterInvestigated;
        for (char dir : options.huntOrder) {
            Coordinate next;
            if (!neighbor(current, dir, next) || cameFromAt(next) != '\0')
                continue;
            char terrain = terrainAt(next);
            if (terrain == '#')
                continue;
            cameFromAt(next) = dir;
            if (terrain == '.') {
                sailContainer.push_back(next);
            } else {
                search(next);
                if (found)
                    break;
            }
        }
    }

    if (options.verbose && !found)
        cout << "Treasure hunt failed\n";
}


void TreasureHunt::printResults() {
    int pathLength = 0;
    if (found) {
        for (Coordinate cell = treasure; cell != start; cell = previous(cell))
            ++pathLength;
    }

    if (options.stats) {
        cout << "--- STATS ---\n"
             << "Starting location: " << start << '\n'
             << "Water locations investigated: " << waterInvestigated << '\n'
             << "Land locations investigated: " << landInvestigated << '\n'
             << "Went ashore: " << wentAshore << '\n';
        if (found) {
            cout << "Path length: " << pathLength << '\n'
                 << "Treasure location: " << treasure << '\n';
        }
        cout << "--- STATS ---\n";
    }

    if (found && options.pathMode == PathMode::kList) {
        char out = '\0';  // the treasure has no next cell
        Coordinate cell = treasure;
        while (cell != start) {
            char in = cameFromAt(cell);
            Coordinate prev = previous(cell);
            cameFromAt(cell) = out;
            out = in;
            cell = prev;
        }
        cameFromAt(start) = out;

        cout << "Sail:\n";
        bool onLand = false;
        for (cell = start; ; neighbor(cell, cameFromAt(cell), cell)) {
            char terrain = terrainAt(cell);
            if (!onLand && terrain != '.' && terrain != '@') {
                onLand = true;
                cout << "Search:\n";
            }
            cout << cell << '\n';
            if (cell == treasure)
                break;
        }
    } else if (found && options.pathMode == PathMode::kMap) {
        char out = cameFromAt(treasure);
        for (Coordinate cell = previous(treasure); cell != start; cell = previous(cell)) {
            char in = cameFromAt(cell);
            bool inVertical = (in == 'N' || in == 'S');
            bool outVertical = (out == 'N' || out == 'S');
            if (inVertical != outVertical)
                terrainAt(cell) = '+';
            else
                terrainAt(cell) = inVertical ? '|' : '-';
            out = in;
        }
        terrainAt(treasure) = 'X';

        for (const vector<char> &row : grid) {
            cout.write(row.data(), size);
            cout << '\n';
        }
    }

    if (found) {
        cout << "Treasure found at " << treasure << " with path length " << pathLength << ".\n";
    } else {
        cout << "No treasure found after investigating " << waterInvestigated + landInvestigated << " locations.\n";
    }
}
