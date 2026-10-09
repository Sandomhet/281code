// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2619B6

#ifndef STOCKMARKET_HPP
#define STOCKMARKET_HPP

#include <cstdint>
#include <functional>
#include <istream>
#include <queue>
#include <vector>


struct Options {
    bool verbose = false;
    bool median = false;
    bool traderInfo = false;
    bool timeTravelers = false;
};


struct Order {
    uint32_t id = 0;      // arrival sequence number, breaks price ties
    uint32_t trader = 0;
    int price = 0;
    mutable int quantity = 0;  // not part of the ordering, so safe to change in a PQ
};


// Lowest price first, then earliest arrival.
struct SellCompare {
    bool operator()(const Order &a, const Order &b) const {
        if (a.price != b.price)
            return a.price > b.price;
        return a.id > b.id;
    }
};


// Highest price first, then earliest arrival.
struct BuyCompare {
    bool operator()(const Order &a, const Order &b) const {
        if (a.price != b.price)
            return a.price < b.price;
        return a.id > b.id;
    }
};


// Running median of match prices: lower half in a max-heap, upper half in a min-heap.
class RunningMedian {
public:
    void push(int value);
    bool empty() const { return lower.empty(); }
    int median() const;

private:
    std::priority_queue<int> lower;
    std::priority_queue<int, std::vector<int>, std::greater<int>> upper;
};


// Best buy-low / sell-high pair seen so far, updated in O(1) per order.
class TimeTraveler {
public:
    void sellOrder(int timestamp, int price);
    void buyOrder(int timestamp, int price);
    bool hasProfit() const { return state == State::kComplete || state == State::kPotential; }

    int buyTime = -1;
    int buyPrice = 0;
    int sellTime = -1;
    int sellPrice = 0;

private:
    enum class State {
        kNoTrades,   // no sell order seen yet
        kCanBuy,     // have a buy point, no profitable sell point yet
        kComplete,   // have a profitable pair
        kPotential,  // have a pair, plus a cheaper buy point that may beat it
    };
    State state = State::kNoTrades;
    int potentialTime = -1;
    int potentialPrice = 0;
};


struct Stock {
    std::priority_queue<Order, std::vector<Order>, SellCompare> sells;
    std::priority_queue<Order, std::vector<Order>, BuyCompare> buys;
};


struct Trader {
    int64_t bought = 0;
    int64_t sold = 0;
    int64_t net = 0;
};


class Market {
public:
    explicit Market(const Options &opts) : options(opts) {}

    void readHeader();
    void run();
    void printSummary() const;

private:
    Options options;
    uint32_t numTraders = 0;
    uint32_t numStocks = 0;
    bool pseudoRandom = false;
    uint32_t seed = 0;
    uint32_t numOrders = 0;
    uint32_t arrivalRate = 0;

    // medians, travelers, and traders are only allocated when their output is requested.
    std::vector<Stock> stocks;
    std::vector<RunningMedian> medians;
    std::vector<TimeTraveler> travelers;
    std::vector<Trader> traders;
    int currentTimestamp = 0;
    uint32_t nextId = 0;
    uint64_t tradesCompleted = 0;

    void processOrders(std::istream &in);
    void matchBuy(uint32_t stockId, Order &buy);
    void matchSell(uint32_t stockId, Order &sell);
    void recordTrade(uint32_t stockId, uint32_t buyer, uint32_t seller, int price, int quantity);
    void printMedians() const;
};

#endif
