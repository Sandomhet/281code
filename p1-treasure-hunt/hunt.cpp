// Project Identifier: 40FB54C86566B9DDEAB902CC80E8CE85C1261AAD
// EECS 281: Project 1 - Treasure Hunt

#include <cstdlib>
#include <iostream>
#include <string>

#include <getopt.h>

#include "TreasureHunt.hpp"
using namespace std;


void printHelp(char *command) {
    cout << "Usage: " << command << " [options] < inputfile\n"
         << "  -h, --help                       Print this message and exit\n"
         << "  -c, --captain <QUEUE|STACK>      Container used while sailing (default STACK)\n"
         << "  -f, --first-mate <QUEUE|STACK>   Container used while searching (default QUEUE)\n"
         << "  -o, --hunt-order <ORDER>         Discovery order, a permutation of NESW\n"
         << "  -v, --verbose                    Print verbose output while searching\n"
         << "  -s, --stats                      Display statistics after the search\n"
         << "  -p, --show-path <M|L>            Show the path as a map or a list\n"
         << flush;
}


Container parseContainer(const char *arg, const char *optName) {
    string s(arg);
    if (s == "STACK")
        return Container::kStack;
    if (s == "QUEUE")
        return Container::kQueue;
    cerr << "Invalid argument to --" << optName << '\n';
    exit(1);
}


bool validHuntOrder(const string &order) {
    if (order.size() != 4)
        return false;
    for (char dir : string("NESW")) {
        int count = 0;
        for (char c : order)
            count += (c == dir);
        if (count != 1)
            return false;
    }
    return true;
}


void getOptions(int argc, char **argv, Options &options) {
    opterr = static_cast<int>(false);
    int choice = 0;
    int index = 0;
    bool pathSeen = false;

    option longOptions[] = {
        {       "help",       no_argument, nullptr, 'h'},
        {    "captain", required_argument, nullptr, 'c'},
        { "first-mate", required_argument, nullptr, 'f'},
        { "hunt-order", required_argument, nullptr, 'o'},
        {    "verbose",       no_argument, nullptr, 'v'},
        {      "stats",       no_argument, nullptr, 's'},
        {  "show-path", required_argument, nullptr, 'p'},
        {      nullptr,                 0, nullptr, '\0'},
    };

    while ((choice = getopt_long(argc, argv, "hc:f:o:vsp:", longOptions, &index)) != -1) {
        switch (choice) {
            case 'h':
                printHelp(*argv);
                exit(0);

            case 'c':
                options.captain = parseContainer(optarg, "captain");
                break;

            case 'f':
                options.firstMate = parseContainer(optarg, "first-mate");
                break;

            case 'o':
                options.huntOrder = optarg;
                if (!validHuntOrder(options.huntOrder)) {
                    cerr << "Invalid argument to --hunt-order\n";
                    exit(1);
                }
                break;

            case 'v':
                options.verbose = true;
                break;

            case 's':
                options.stats = true;
                break;

            case 'p': {
                if (pathSeen) {
                    cerr << "Specify --show-path only once\n";
                    exit(1);
                }
                pathSeen = true;
                string arg(optarg);
                if (arg == "M") {
                    options.pathMode = PathMode::kMap;
                } else if (arg == "L") {
                    options.pathMode = PathMode::kList;
                } else {
                    cerr << "Invalid argument to --show-path\n";
                    exit(1);
                }
                break;
            }

            default:
                cerr << "Unknown option\n";
                exit(1);
        }
    }
}


int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);

    Options options;
    getOptions(argc, argv, options);

    TreasureHunt hunt(options);
    hunt.readInput();
    hunt.hunt();
    hunt.printResults();
    return 0;
}
