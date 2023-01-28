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
    if (!mClob.empty()) {
        for (auto& node: mClob) {
            auto idsEqual = [&](Order order){
                if (order.id == id) return true; 
                return false;
            };
            // Look for the id in the buy orders
            const auto& buy_trade_it = std::find_if(node.second->buy_orders.begin(),
                                                    node.second->buy_orders.end(), idsEqual);
            if (buy_trade_it != node.second->buy_orders.end()) {
                node.second->buy_orders.erase(*buy_trade_it);
                mActiveOrderIds.erase(buy_trade_it->id);
                processCurrentOrders();
                return;
            }
            // Look for the id in the sell orders
            const auto& sell_trade_it = std::find_if(node.second->sell_orders.begin(),
                                                     node.second->sell_orders.end(), idsEqual);
            if (sell_trade_it != node.second->sell_orders.end()) {
                node.second->buy_orders.erase(*sell_trade_it);
                mActiveOrderIds.erase(sell_trade_it->id);
                processCurrentOrders();
                return;
            }
        }
    }
    throw std::runtime_error(std::string("Error: Cannot pull order #" + std::to_string(id)));
}

bool MatchingEngine::amend(std::set<Order>& order_set, const std::string& symbol, order::Id id, double price, int volume) {
    const auto& trade_it = std::find_if(order_set.begin(), 
                                        order_set.end(), 
                                        [&](Order order) {
                                                            if (order.id == id) 
                                                                return true; 
                                                            return false;
                                                        });
    if (trade_it != order_set.end()) {
        auto old_id = trade_it->id;
        auto old_side = trade_it->side;
        auto old_timestamp = trade_it->timestamp;
        if (trade_it->price == price && trade_it->volume > volume) {
            order_set.erase(*trade_it);
            addOrder(symbol, {old_id, old_side, price, volume, old_timestamp});
        } else {
            order_set.erase(*trade_it);
            auto new_ts = std::chrono::system_clock::now().time_since_epoch().count();
            addOrder(symbol, {old_id, old_side, price, volume, new_ts, new_ts});
        }
        return true;
    }
    return false;
}

void MatchingEngine::amendOrder(order::Id id, double price, int volume) {
    if (!mClob.empty()) {
        for (auto& node: mClob) {
            if (!amend(node.second->buy_orders, node.first, id, price, volume) && 
                !amend(node.second->sell_orders, node.first, id, price, volume)) {
                    throw std::runtime_error(std::string("Error: Cannot amend order #" + std::to_string(id)));
                }
        }
    }
}

void MatchingEngine::processCurrentOrders() {
    for (const auto& node: mClob) {
        while (!node.second->buy_orders.empty() && !node.second->sell_orders.empty() && (
               std::prev(node.second->buy_orders.end())->price >= node.second->sell_orders.begin()->price
            )) {
                const auto& buy_order = std::prev(node.second->buy_orders.end()); // Best bid
                const auto& sell_order = node.second->sell_orders.begin(); // Lowest ask
                order::Id agressive_order_id = sell_order->id;
                order::Id passive_order_id = buy_order->id;
                if (buy_order->last_updated > sell_order->last_updated) {
                    std::swap(agressive_order_id, passive_order_id);
                }
                if (buy_order->volume == sell_order->volume) {
                    node.second->buy_orders.erase(buy_order);
                    node.second->sell_orders.erase(sell_order);
                    addTradeToHistory(node.first, buy_order->price, sell_order->volume, agressive_order_id, passive_order_id);
                } else if (buy_order->volume > sell_order->volume) {
                    Order updated_buy_order(*buy_order);
                    int stocks_exchanged = buy_order->volume - sell_order->volume;
                    updated_buy_order.volume = stocks_exchanged;
                    node.second->buy_orders.erase(buy_order);
                    node.second->buy_orders.insert(updated_buy_order);
                    node.second->sell_orders.erase(sell_order);
                    addTradeToHistory(node.first, buy_order->price, sell_order->volume, agressive_order_id, passive_order_id);
                } else {
                    Order updated_sell_order(*sell_order);
                    updated_sell_order.volume = sell_order->volume - buy_order->volume;
                    node.second->sell_orders.erase(sell_order);
                    node.second->sell_orders.insert(updated_sell_order);
                    node.second->buy_orders.erase(buy_order);                   
                    addTradeToHistory(node.first, buy_order->price, buy_order->volume, agressive_order_id, passive_order_id);
                }
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
    for (const auto& node: mClob) {
        // Add separator
        std::string symbol_separator("===");
        symbol_separator.append(node.first);
        symbol_separator.append("===");
        result.push_back(symbol_separator);
        
        // Add remaining unprocessed orders
        std::map<double, int> remaining_sell_orders;
        std::map<double, int> remaining_buy_orders;
        std::string remaining_orders;
        while (!node.second->buy_orders.empty() || !node.second->sell_orders.empty()) {
            const auto& best_buy_order = node.second->buy_orders.end();
            if (!node.second->buy_orders.empty()) {
                const auto& next_buy_order = std::prev(best_buy_order);
                remaining_buy_orders[next_buy_order->price] += next_buy_order->volume;
                node.second->buy_orders.erase(next_buy_order);
            }
            if (node.second->sell_orders.size() >= 1) {
                const auto& next_sell_order = node.second->sell_orders.begin();
                remaining_sell_orders[next_sell_order->price] += next_sell_order->volume;
                node.second->sell_orders.erase(next_sell_order);
            }            
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
