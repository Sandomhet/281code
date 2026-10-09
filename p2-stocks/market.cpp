// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2619B6
// EECS 281: Project 2A - Stock Market Simulation

#include <cstdlib>
#include <iostream>

#include <getopt.h>

#include "StockMarket.hpp"
using namespace std;


void printHelp(char *command) {
    cout << "Usage: " << command << " [options] < inputfile\n"
         << "Simulates an electronic stock exchange, matching buy and sell orders.\n"
         << "  -h, --help             Print this message and exit\n"
         << "  -v, --verbose          Print each trade as it is completed\n"
         << "  -m, --median           Print median match prices at each timestamp\n"
         << "  -i, --trader_info      Print per-trader totals at end of day\n"
         << "  -t, --time_travelers   Print the best time-traveler trade for each stock\n"
         << flush;
}


void getOptions(int argc, char **argv, Options &options) {
    opterr = static_cast<int>(false);
    int choice = 0;
    int index = 0;

    option longOptions[] = {
        {          "help", no_argument, nullptr, 'h'},
        {       "verbose", no_argument, nullptr, 'v'},
        {        "median", no_argument, nullptr, 'm'},
        {   "trader_info", no_argument, nullptr, 'i'},
        {"time_travelers", no_argument, nullptr, 't'},
        {         nullptr,           0, nullptr, '\0'},
    };

    while ((choice = getopt_long(argc, argv, "hvmit", longOptions, &index)) != -1) {
        switch (choice) {
            case 'h':
                printHelp(*argv);
                exit(0);

            case 'v':
                options.verbose = true;
                break;

            case 'm':
                options.median = true;
                break;

            case 'i':
                options.traderInfo = true;
                break;

            case 't':
                options.timeTravelers = true;
                break;

            default:
                cerr << "Error: Unknown command line option\n";
                exit(1);
        }
    }
}


int main(int argc, char *argv[]) {
    ios_base::sync_with_stdio(false);

    Options options;
    getOptions(argc, argv, options);

    Market market(options);
    market.readHeader();
    market.run();
    market.printSummary();
    return 0;
}
