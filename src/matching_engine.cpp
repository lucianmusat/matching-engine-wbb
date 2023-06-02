#include "matching_engine.hpp"
#include "string_utils.hpp"

#include <chrono>
#include <algorithm>


std::vector<std::string> MatchingEngine::processOrders(const std::vector<std::string>& input) {
    if (input.empty()) return {};
    for (const auto& command: input) {
        auto command_arguments = utils::splitCommands(command);
        if (command_arguments.empty()) {
            throw std::runtime_error("Error: Invalid command!");
        }
        if (command_arguments.at(0) == "INSERT") {
            insert_order(command_arguments);
        } else if (command_arguments.at(0) == "AMEND") {
            amend_order(command_arguments);
        } else if (command_arguments.at(0) == "PULL") {
            pullOrder(std::stoi(command_arguments.at(1)));
        } else {
            throw std::runtime_error("Error: Invalid command!");
        }
    }       
    return getFinalResult();
}

void MatchingEngine::insert_order(const std::vector<std::string>& command_arguments) {
    order::Id orderId = std::stoul(command_arguments.at(1));
    std::string symbol = command_arguments.at(2);
    order::Side side = command_arguments.at(3) == "BUY" ? order::Side::buy : order::Side::sell;
    double price = std::stod(command_arguments.at(4));
    int volume = std::stoi(command_arguments.at(5));
    if (utils::isValidPrice(command_arguments.at(4)) && mActiveOrderIds.find(orderId) == mActiveOrderIds.end()) {
        addOrder(symbol, {orderId, side, price, volume});
    } else throw std::runtime_error("Error: Invalid INSERT command!");
}

void MatchingEngine::amend_order(const std::vector<std::string>& command_arguments) {
    order::Id orderId = std::stoul(command_arguments.at(1));
    double price = std::stod(command_arguments.at(2));
    int volume = std::stoi(command_arguments.at(3));
    if (utils::isValidPrice(command_arguments.at(2))) {
        amendOrder(orderId, price, volume);
    } else throw std::runtime_error("Error: Invalid AMEND command!");
}

void MatchingEngine::addOrder(const std::string& symbol, Order order) {
    if (!mClob[symbol]) {
        mClob[symbol] = std::make_unique<engine::TradeNode>();
    }
    if (order.side == order::Side::buy) {
        mClob[symbol]->buy_orders.insert(order);
    } else {
        mClob[symbol]->sell_orders.insert(order);
    }
    mActiveOrderIds.insert(order.id);
    processCurrentOrders();
}

void MatchingEngine::pullOrder(order::Id id) {
    // If you maintain some mapping of orderID to store the map where the actual order is stored, you don't need these inefficient iterations
    for (auto& [_, node]: mClob) {
        auto idsEqual = [&](Order order){
            return order.id == id;
        };
        // Removed duplicated code. Maybe there's a nicer way to do it than this for loop, but it at least deduplicates
        // There was also a bug in the old code where you used either buy or sell orders in the wrong place. This fixes that
        for (auto& orders : {&node->buy_orders, &node->sell_orders})
        {
            const auto& trade_it = std::find_if(orders->begin(), orders->end(), idsEqual);
            if (trade_it != orders->end()) {
                orders->erase(*trade_it);
                mActiveOrderIds.erase(trade_it->id);
                processCurrentOrders(); // Shouldn't be a need to rerun the orders right?
                return;
            }
        }
    }
    throw std::runtime_error(std::string("Error: Cannot pull order #" + std::to_string(id)));
}

// There are multiple functions in this class, all with amend in their name. What distinguishes them? More explicit naming can help here
bool MatchingEngine::amend(std::set<Order>& order_set, const std::string& symbol, order::Id id, double price, int volume) {
    // You could inline this function using a similar tricks as above, iterating over the two different types of orders
    const auto& trade_it = std::find_if(order_set.begin(), order_set.end(), [&](Order order) { return order.id == id; });
    if (trade_it != order_set.end()) {
        auto new_trade_id = trade_it->id;
        auto new_trade_side = trade_it->side;
        auto new_trade_ts = trade_it->timestamp;
        if (trade_it->price != price || trade_it->volume <= volume)
        {
            new_trade_ts = std::chrono::system_clock::now().time_since_epoch().count();
        }

        order_set.erase(*trade_it);
        // Why call this addOrder function here, while below in processCurrentOrders you update the order, then remove/reinsert in the set directly? Either could be fine, but not both
        addOrder(symbol, {new_trade_id, new_trade_side, price, volume, new_trade_ts});
        return true;
    }
    return false;
}

void MatchingEngine::amendOrder(order::Id id, double price, int volume) {
    for (auto& [symbol, node]: mClob) {
        if (!amend(node->buy_orders, symbol, id, price, volume) &&
            !amend(node->sell_orders, symbol, id, price, volume)) {
                throw std::runtime_error(std::string("Error: Cannot amend order #" + std::to_string(id)));
            }
    }
}

void MatchingEngine::processCurrentOrders() {
    for (auto& [symbol, node]: mClob) {
        while (!node->buy_orders.empty() && !node->sell_orders.empty()) {
            const auto best_bid = std::prev(node->buy_orders.end());
            const auto lowest_ask = node->sell_orders.begin();
            if (best_bid->price < lowest_ask->price)
                break;

            order::Id agressive_order_id = lowest_ask->id;
            order::Id passive_order_id = best_bid->id;
            auto price_traded = best_bid->price; // This might have been a bug before? Price traded depends on which side is aggressive and which passive. I think I did this the right way around but it's late, you get the point
            auto volume_traded = std::min(best_bid->volume, lowest_ask->volume);
            if (best_bid->last_updated > lowest_ask->last_updated) {
                std::swap(agressive_order_id, passive_order_id);
                price_traded = lowest_ask->price;
            }

            Order updated_best_bid(*best_bid);
            Order updated_lowest_ask(*lowest_ask);
            updated_best_bid.volume -= volume_traded;
            updated_lowest_ask.volume -= volume_traded;

            node->buy_orders.erase(best_bid);
            node->sell_orders.erase(lowest_ask);
            if (updated_best_bid.volume > 0)
                node->buy_orders.insert(updated_best_bid);
            if (updated_lowest_ask.volume > 0)
                node->sell_orders.insert(updated_lowest_ask);

            addTradeToHistory(symbol, price_traded, volume_traded, agressive_order_id, passive_order_id);
        }
    }
}

void MatchingEngine::addTradeToHistory(const std::string& symbol, double price, int volume , order::Id agressive_id, order::Id passive_id) {
    std::string trade(symbol);
    trade.append(",");
    trade.append(utils::dropTrailingZeroes(std::to_string(price)));
    trade.append(",");
    trade.append(std::to_string(volume));
    trade.append(",");
    trade.append(std::to_string(agressive_id));
    trade.append(",");
    trade.append(std::to_string(passive_id));
    mListOfTrades.push_back(trade);
}

std::vector<std::string> MatchingEngine::getFinalResult() {
    std::vector<std::string> result(mListOfTrades);
    for (auto& [symbol, node]: mClob) {
        // Add separator
        std::string symbol_separator("===");
        symbol_separator.append(symbol);
        symbol_separator.append("===");
        result.push_back(symbol_separator);
        
        // Add remaining unprocessed orders
        std::map<double, int> remaining_sell_orders;
        std::map<double, int> remaining_buy_orders;
        std::string remaining_orders;
        for (const auto& order: node->buy_orders)
        {
            remaining_buy_orders[order.price] += order.volume;
        }
        for (const auto& order: node->sell_orders)
        {
            remaining_sell_orders[order.price] += order.volume;
        }
        auto max_len = std::max(remaining_sell_orders.size(), remaining_buy_orders.size());
        for (auto i=0; i<max_len; i++) {
            if (remaining_buy_orders.size() > i) {
                auto buy_it = remaining_buy_orders.rbegin();
                std::advance(buy_it, i);
                remaining_orders.append(utils::dropTrailingZeroes(std::to_string(buy_it->first)));
                remaining_orders.append(",");
                remaining_orders.append(std::to_string(buy_it->second));
            } else remaining_orders.append(",");
            remaining_orders.append(",");
            if (remaining_sell_orders.size() > i) {
                auto sell_it = remaining_sell_orders.begin();
                std::advance(sell_it, i);
                remaining_orders.append(utils::dropTrailingZeroes(std::to_string(sell_it->first)));
                remaining_orders.append(",");
                remaining_orders.append(std::to_string(sell_it->second));
            } else remaining_orders.append(",");
            result.push_back(remaining_orders);
            remaining_orders = "";
        }
    }
    return result;
}
