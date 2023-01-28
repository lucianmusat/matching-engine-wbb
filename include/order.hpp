#pragma once

#include <cstdint>
#include <ctime>
#include <string>

namespace order {

enum class Side {
    buy,
    sell
};

using Id = uint64_t;
    
} // order namespace

/*! \brief Struct that represents a buy or sell order in the market.
 *
 *  Id's are provided, and timestamps are generated if not provided.
 */
struct Order {
    /* 
    *  \brief Main constructor. Creates a timestamp and last_updated is intitialized with 0.
    */
    Order(order::Id, order::Side, double limit_price, int volume);

    /* 
    *  \brief Custom timestamp constructor. last_updated is intitialized with 0. 
    *         Used to amend an order without losing time priority. 
    */
    Order(order::Id, order::Side, double limit_price, int volume, std::time_t timestamp);
    
    /* 
    *  \brief Custom timestamp and last_updated timestamp constructor. 
    *         Used to amend an order. 
    */
    Order(order::Id id, order::Side side, double limit_price, int volume, std::time_t timestamp, std::time_t amend_ts);
    
    bool operator<(const Order& other) const;
    bool operator==(const Order& other) const;
    bool operator!=(const Order& other) const;
            
    order::Id id;
    order::Side side;
    double price;
    int volume;
    std::time_t timestamp;
    std::time_t last_updated;
};
