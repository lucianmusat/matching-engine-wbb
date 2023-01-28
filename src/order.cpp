#include "order.hpp"

#include <chrono>
#include <iostream>

Order::Order(order::Id provided_id, order::Side provided_side, double limit_price, int provided_volume)
    : id(provided_id)
    , side(provided_side)
    , price(limit_price)
    , volume(provided_volume)
    , timestamp(std::chrono::system_clock::now().time_since_epoch().count())
    , last_updated(0)
{}

Order::Order(order::Id provided_id, order::Side provided_side, double limit_price, int provided_volume, std::time_t ts)
    : id(provided_id)
    , side(provided_side)
    , price(limit_price)
    , volume(provided_volume)
    , timestamp(ts)
    , last_updated(0)
{}

Order::Order(order::Id provided_id, order::Side provided_side, double limit_price, int provided_volume, std::time_t ts, std::time_t amend_ts)
    : id(provided_id)
    , side(provided_side)
    , price(limit_price)
    , volume(provided_volume)
    , timestamp(ts)
    , last_updated(amend_ts)
{}

bool Order::operator<(const Order& other) const {
    if (this->side == order::Side::buy)
        return (this->price < other.price) ||
            (this->price == other.price && this->timestamp > other.timestamp);
    return (this->price < other.price) ||
        (this->price == other.price && this->timestamp < other.timestamp);
}

bool Order::operator==(const Order& other) const {
    return (this->id == other.id && this->price == other.price && this->timestamp == other.timestamp);
}

bool Order::operator!=(const Order& other) const {
    return !(*this == other);
}
