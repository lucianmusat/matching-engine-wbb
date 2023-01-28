#pragma once

#include <sstream>
#include <iomanip>

namespace utils {

    inline bool isValidDouble(const std::string& str) {
        try {
            std::stod(str);     
        } catch (const std::invalid_argument&) {
            return false;
        }
        return true;
    }

    inline bool isValidPrice(const std::string& price) {
        if (!isValidDouble(price)) return false;
        auto found_location = price.find('.');
        if (found_location != std::string::npos) {
            std::string substring(price.begin() + found_location + 1, price.end());
            if (substring.size() > 4) return false;
        }
        return true;
    }

    inline std::vector<std::string> splitCommands(const std::string& order) {
        std::stringstream order_stream(order);
        std::string segment;
        std::vector<std::string> commands_list;
        while(std::getline(order_stream, segment, ',')) {
        commands_list.push_back(segment);
        }
        return commands_list;
    }
    
    inline std::string dropTrailingZeroes(const std::string& value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << value;
        std::string str = ss.str();
        if (str.find('.') != std::string::npos) {
            str = str.substr(0, str.find_last_not_of('0') + 1);
            // If the decimal point is now the last character, remove that as well
            if(str.find('.') == str.size() - 1) {
                str = str.substr(0, str.size() - 1);
            }
        }
        return str;
    }

} // utils namespace
