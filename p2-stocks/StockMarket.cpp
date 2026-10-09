// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A2619B6

#include "StockMarket.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "P2random.hpp"

using namespace std;


void RunningMedian::push(int value) {
    if (lower.empty() || value <= lower.top())
        lower.push(value);
    else
        upper.push(value);

    // Keep lower.size() == upper.size() or upper.size() + 1.
    if (lower.size() > upper.size() + 1) {
        upper.push(lower.top());
        lower.pop();
    } else if (upper.size() > lower.size()) {
        lower.push(upper.top());
        upper.pop();
    }
}


int RunningMedian::median() const {
    // The reference solution sums in 32 bits, so wrap the same way (without signed-overflow UB).
    if (lower.size() == upper.size())
        return static_cast<int32_t>(static_cast<uint32_t>(lower.top()) + static_cast<uint32_t>(upper.top())) / 2;
    return lower.top();
}


// A sell order is a chance for the time traveler to buy.
void TimeTraveler::sellOrder(int timestamp, int price) {
    switch (state) {
    case State::kNoTrades:
        buyTime = timestamp;
        buyPrice = price;
        state = State::kCanBuy;
        break;
    case State::kCanBuy:
        if (price < buyPrice) {
            buyTime = timestamp;
            buyPrice = price;
        }
        break;
    case State::kComplete:
        if (price < buyPrice) {
            potentialTime = timestamp;
            potentialPrice = price;
            state = State::kPotential;
        }
        break;
    case State::kPotential:
        if (price < potentialPrice) {
            potentialTime = timestamp;
            potentialPrice = price;
        }
        break;
    }
}


// A buy order is a chance for the time traveler to sell.
void TimeTraveler::buyOrder(int timestamp, int price) {
    switch (state) {
    case State::kNoTrades:
        break;
    case State::kCanBuy:
        if (price > buyPrice) {
            sellTime = timestamp;
            sellPrice = price;
            state = State::kComplete;
        }
        break;
    case State::kComplete:
        if (price > sellPrice) {
            sellTime = timestamp;
            sellPrice = price;
        }
        break;
    case State::kPotential:
        if (static_cast<int64_t>(price) - potentialPrice > static_cast<int64_t>(sellPrice) - buyPrice) {
            buyTime = potentialTime;
            buyPrice = potentialPrice;
            sellTime = timestamp;
            sellPrice = price;
            state = State::kComplete;
        }
        break;
    }
}


void Market::readHeader() {
    string junk;
    string mode;
    getline(cin, junk);  // COMMENT: ...
    cin >> junk >> mode;
    cin >> junk >> numTraders;
    cin >> junk >> numStocks;

    if (mode == "PR") {
        pseudoRandom = true;
        cin >> junk >> seed;
        cin >> junk >> numOrders;
        cin >> junk >> arrivalRate;
    }

    stocks.resize(numStocks);
    if (options.median)
        medians.resize(numStocks);
    if (options.timeTravelers)
        travelers.resize(numStocks);
    if (options.traderInfo)
        traders.resize(numTraders);
}


void Market::run() {
    cout << "Processing orders...\n";

    P2random::GenStream gen;
    if (pseudoRandom) {
        gen.init(seed, numTraders, numStocks, numOrders, arrivalRate);
        processOrders(gen);
    } else {
        processOrders(cin);
    }

    if (options.median)
        printMedians();
}


void Market::processOrders(istream &in) {
    int timestamp = 0;
    string intent;
    char c = '\0';
    int traderId = 0;
    int stockId = 0;
    int price = 0;
    int quantity = 0;

    while (in >> timestamp >> intent >> c >> traderId >> c >> stockId >> c >> price >> c >> quantity) {
        if (timestamp < currentTimestamp) {
            cerr << "Error: Decreasing timestamp\n";
            exit(1);
        }
        if (traderId < 0 || static_cast<uint32_t>(traderId) >= numTraders) {
            cerr << "Error: Invalid trader ID\n";
            exit(1);
        }
        if (stockId < 0 || static_cast<uint32_t>(stockId) >= numStocks) {
            cerr << "Error: Invalid stock ID\n";
            exit(1);
        }
        if (price <= 0) {
            cerr << "Error: Invalid price\n";
            exit(1);
        }
        if (quantity <= 0) {
            cerr << "Error: Invalid quantity\n";
            exit(1);
        }

        if (timestamp != currentTimestamp) {
            if (options.median)
                printMedians();
            currentTimestamp = timestamp;
        }

        Order order;
        order.id = nextId++;
        order.trader = static_cast<uint32_t>(traderId);
        order.price = price;
        order.quantity = quantity;

        auto stock = static_cast<uint32_t>(stockId);
        if (intent == "BUY") {
            if (options.timeTravelers)
                travelers[stock].buyOrder(timestamp, price);
            matchBuy(stock, order);
        } else {
            if (options.timeTravelers)
                travelers[stock].sellOrder(timestamp, price);
            matchSell(stock, order);
        }
    }
}


void Market::matchBuy(uint32_t stockId, Order &buy) {
    Stock &stock = stocks[stockId];
    while (buy.quantity > 0 && !stock.sells.empty() && stock.sells.top().price <= buy.price) {
        const Order &sell = stock.sells.top();
        int amount = min(buy.quantity, sell.quantity);
        // The sell order arrived first, so the trade happens at its price.
        recordTrade(stockId, buy.trader, sell.trader, sell.price, amount);
        buy.quantity -= amount;
        sell.quantity -= amount;
        if (sell.quantity == 0)
            stock.sells.pop();
    }
    if (buy.quantity > 0)
        stock.buys.push(buy);
}


void Market::matchSell(uint32_t stockId, Order &sell) {
    Stock &stock = stocks[stockId];
    while (sell.quantity > 0 && !stock.buys.empty() && stock.buys.top().price >= sell.price) {
        const Order &buy = stock.buys.top();
        int amount = min(sell.quantity, buy.quantity);
        // The buy order arrived first, so the trade happens at its price.
        recordTrade(stockId, buy.trader, sell.trader, buy.price, amount);
        sell.quantity -= amount;
        buy.quantity -= amount;
        if (buy.quantity == 0)
            stock.buys.pop();
    }
    if (sell.quantity > 0)
        stock.sells.push(sell);
}


void Market::recordTrade(uint32_t stockId, uint32_t buyer, uint32_t seller, int price, int quantity) {
    ++tradesCompleted;

    if (options.verbose) {
        cout << "Trader " << buyer << " purchased " << quantity << " shares of Stock " << stockId
             << " from Trader " << seller << " for $" << price << "/share\n";
    }
    if (options.median)
        medians[stockId].push(price);
    if (options.traderInfo) {
        int64_t value = static_cast<int64_t>(price) * quantity;
        traders[buyer].bought += quantity;
        traders[buyer].net -= value;
        traders[seller].sold += quantity;
        traders[seller].net += value;
    }
}


void Market::printMedians() const {
    for (uint32_t i = 0; i < numStocks; ++i) {
        if (medians[i].empty())
            continue;
        cout << "Median match price of Stock " << i << " at time " << currentTimestamp << " is $"
             << medians[i].median() << '\n';
    }
}


void Market::printSummary() const {
    cout << "---End of Day---\n"
         << "Trades Completed: " << tradesCompleted << '\n';

    if (options.traderInfo) {
        cout << "---Trader Info---\n";
        for (uint32_t i = 0; i < numTraders; ++i) {
            cout << "Trader " << i << " bought " << traders[i].bought << " and sold " << traders[i].sold
                 << " for a net transfer of $" << traders[i].net << '\n';
        }
    }

    if (options.timeTravelers) {
        cout << "---Time Travelers---\n";
        for (uint32_t i = 0; i < numStocks; ++i) {
            const TimeTraveler &t = travelers[i];
            if (t.hasProfit()) {
                cout << "A time traveler would buy Stock " << i << " at time " << t.buyTime << " for $"
                     << t.buyPrice << " and sell it at time " << t.sellTime << " for $" << t.sellPrice
                     << '\n';
            } else {
                cout << "A time traveler could not make a profit on Stock " << i << '\n';
            }
        }
    }
}
