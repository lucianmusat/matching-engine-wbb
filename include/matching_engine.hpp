#pragma once

#include <vector>
#include <string>
#include <set>
#include <memory>
#include <map>

#include "order.hpp"

namespace engine {
    
    /*! \brief Struct that holds all the buy and sell orders for a particular symbol.
    *
    * Uses std::set instead of std::priority_queue here because the priority_queue lacks iteration.
    */
    struct TradeNode {
        TradeNode() = default;
        
        std::set<Order> buy_orders;
        std::set<Order> sell_orders;
    };
    
} // engine namespace


/*! \brief Class that represents a basic market and provides a matching engine.
 *
 *  It has one public method, processOrders(), which takes the string input, chops it into
 *  individual orders and adds them to the proper order queue. 
 *  Every time an order is added, deleted or modified it checks again for new possible trades.
 */
class MatchingEngine {
    public:        
        MatchingEngine() = default;
        
        /*! 
        *  \brief Takes the input argument, parses each line and calls the appropriate method for each command.
        *  
        * \throws std::runtime_error.
        *
        * \ret Returns the output in the expected format
        */         
        std::vector<std::string> processOrders(const std::vector<std::string>& input);
    
    private:
        // We could use unordered_map but it needs to be in alphabetical order
        std::map<std::string, std::unique_ptr<engine::TradeNode>> mClob {};
        std::vector<std::string> mListOfTrades {};
        std::set<order::Id> mActiveOrderIds;
        
        /*! 
        *  \brief Add a buy or sell order in the market.
        */
        void addOrder(const std::string& symbol, Order);

        /*! 
        *  \brief Remove a buy or sell order from the market.
        */
        void pullOrder(order::Id);
        
        /*! 
        *  \brief Modify the price or volume of an existing order.
        *
        *   If the price is modified, or the volume increased, then the order
        *   will lose it's time priority.
        *
        */        
        void amendOrder(order::Id, double price, int volume); 

        /*! 
        *  \brief Modify the price or volume of an either buy or sell existing order.
        *
        *   \ret Returns false if no order with that id was found, true otherwise.
        */
        bool amend(std::set<Order>& order_set, const std::string& symbol, order::Id id, double price, int volume);
        
        /*! 
        *  \brief Matching engine.
        *
        *   Iterates over all the existing symbols and looks if any trade can happen.
        *   A trade happens when the top of the buy stack and bottom of the sell stack match in price.
        *   Buy and sell stacks are always ordered by price and timestamp.
        *   When a match happens the volumes are updated accordingly, and for the orders that has been
        *   fulfilled the stack is popped.
        *
        *   This function would be a candidate for adding to it's own thread.
        */
        void processCurrentOrders();
        
        /*! 
        *  \brief When a trade took place, it adds it to the list of trades.
        */    
        void addTradeToHistory(const std::string& symbol, double price, int volume , order::Id agressive_id, order::Id passive_id);
        std::vector<std::string> getFinalResult();
        
        /*! 
        *  \brief Takes string arguments from the input, creates an Order and calls addOrder on it.
        *  
        * \throws std::runtime_error.
        */ 
        void insert_order(const std::vector<std::string>&);
        
        /*! 
        *  \brief Takes string arguments from the input and calls amendOrder.
        *  
        * \throws std::runtime_error.
        */ 
        void amend_order(const std::vector<std::string>&);
};
