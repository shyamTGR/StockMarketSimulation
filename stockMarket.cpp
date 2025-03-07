#include <iostream>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <algorithm>

#define NUM_TICKERS 1024
#define MAX_ORDERS_PER_SIDE 1000


enum OrderType { BUY, SELL };

//order-side structure using fixed-size arrays
struct OrderSide {
    int orderId[MAX_ORDERS_PER_SIDE];
    std::atomic<int> quantity[MAX_ORDERS_PER_SIDE];
    double price[MAX_ORDERS_PER_SIDE];
    std::atomic<bool> active[MAX_ORDERS_PER_SIDE];
    std::atomic<int> count;  // number of orders added so far

    OrderSide() : count(0) {
        for (int i = 0; i < MAX_ORDERS_PER_SIDE; i++) {
            active[i].store(false);
            quantity[i].store(0);
        }
    }
};


struct OrderBook {
    OrderSide buy;
    OrderSide sell;
};

// Global order books
OrderBook orderBooks[NUM_TICKERS];

// Global order id generator.
std::atomic<int> globalOrderId(0);

// addOrder adds an order (buy or sell) for a given ticker.
void addOrder(OrderType type, int ticker, int quantity, double price) {
    int orderId = globalOrderId.fetch_add(1);
    if (type == BUY) {
        int idx = orderBooks[ticker].buy.count.fetch_add(1);
        if (idx < MAX_ORDERS_PER_SIDE) {
            orderBooks[ticker].buy.orderId[idx] = orderId;
            orderBooks[ticker].buy.quantity[idx].store(quantity);
            orderBooks[ticker].buy.price[idx] = price;
            orderBooks[ticker].buy.active[idx].store(true);
        } else {
            std::cout << "Buy order book full for ticker " << ticker << std::endl;
        }
    } else {  // SELL
        int idx = orderBooks[ticker].sell.count.fetch_add(1);
        if (idx < MAX_ORDERS_PER_SIDE) {
            orderBooks[ticker].sell.orderId[idx] = orderId;
            orderBooks[ticker].sell.quantity[idx].store(quantity);
            orderBooks[ticker].sell.price[idx] = price;
            orderBooks[ticker].sell.active[idx].store(true);
        } else {
            std::cout << "Sell order book full for ticker " << ticker << std::endl;
        }
    }
}

// matchOrder scans the order book for a given ticker. It finds the active buy order with the highest price and the active sell order with the lowest price. If the highest buy price is greater than or equal to the lowest sell price, it executes a trade.
void matchOrder(int ticker) {
    OrderBook &ob = orderBooks[ticker];
    int buyCount = ob.buy.count.load();
    int sellCount = ob.sell.count.load();
    int bestBuyIndex = -1;
    double bestBuyPrice = -1.0;
    // O(n) scan of buy orders.
    for (int i = 0; i < buyCount; i++) {
        if (ob.buy.active[i].load() && ob.buy.quantity[i].load() > 0 && ob.buy.price[i] > bestBuyPrice) {
            bestBuyPrice = ob.buy.price[i];
            bestBuyIndex = i;
        }
    }
    int bestSellIndex = -1;
    double bestSellPrice = 1e9;
    // O(n) scan of sell orders.
    for (int i = 0; i < sellCount; i++) {
        if (ob.sell.active[i].load() && ob.sell.quantity[i].load() > 0 && ob.sell.price[i] < bestSellPrice) {
            bestSellPrice = ob.sell.price[i];
            bestSellIndex = i;
        }
    }
    // If there is a matching pair, execute the trade.
    if (bestBuyIndex != -1 && bestSellIndex != -1 && bestBuyPrice >= bestSellPrice) {
         int tradeQuantity = std::min(ob.buy.quantity[bestBuyIndex].load(),
                                      ob.sell.quantity[bestSellIndex].load());
         ob.buy.quantity[bestBuyIndex].fetch_sub(tradeQuantity);
         ob.sell.quantity[bestSellIndex].fetch_sub(tradeQuantity);
         if (ob.buy.quantity[bestBuyIndex].load() == 0)
              ob.buy.active[bestBuyIndex].store(false);
         if (ob.sell.quantity[bestSellIndex].load() == 0)
              ob.sell.active[bestSellIndex].store(false);
         std::cout << "Trade executed for ticker " << ticker 
                   << ": " << tradeQuantity << " shares at price " << bestSellPrice 
                   << " (Buy Order ID: " << ob.buy.orderId[bestBuyIndex]
                   << ", Sell Order ID: " << ob.sell.orderId[bestSellIndex] << ")" << std::endl;
    }
}

// A wrapper function that randomly adds orders to simulate active stock transactions. This function is intended to run on multiple threads.
void addOrdersRandomly(int numOrders, int threadId) {
    std::default_random_engine generator(std::random_device{}());
    std::uniform_int_distribution<int> tickerDist(0, NUM_TICKERS - 1);
    std::uniform_int_distribution<int> quantityDist(1, 100);
    std::uniform_real_distribution<double> priceDist(10.0, 1000.0);
    std::uniform_int_distribution<int> orderTypeDist(0, 1); // 0 for BUY, 1 for SELL

    for (int i = 0; i < numOrders; i++) {
        int ticker = tickerDist(generator);
        int quantity = quantityDist(generator);
        double price = priceDist(generator);
        OrderType type = (orderTypeDist(generator) == 0) ? BUY : SELL;
        addOrder(type, ticker, quantity, price);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "Adder thread " << threadId << " finished adding orders." << std::endl;
}

// This function continuously scans all tickers and calls matchOrder on each. It runs in its own thread.
void matchOrdersContinuously() {
    while (true) {
        for (int ticker = 0; ticker < NUM_TICKERS; ticker++) {
            matchOrder(ticker);
        }
        // Pause briefly.
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

int main() {
    // Launch a thread that continuously matches orders.
    std::thread matcher(matchOrdersContinuously);

    // Launch several threads that add orders concurrently.
    const int numAdderThreads = 4;
    const int ordersPerThread = 1000;
    std::thread adders[numAdderThreads];
    for (int i = 0; i < numAdderThreads; i++) {
        adders[i] = std::thread(addOrdersRandomly, ordersPerThread, i);
    }
    for (int i = 0; i < numAdderThreads; i++) {
        adders[i].join();
    }
  
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::exit(0);
    matcher.join(); 
    return 0;
}
