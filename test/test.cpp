#include "main.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch_all.hpp>

#include <iostream>

TEST_CASE("base buy") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,NVDA,BUY,172.5,200");
    input.emplace_back("INSERT,2,NVDA,SELL,172.5,200");

    auto result = run(input);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "NVDA,172.5,200,2,1");
    CHECK(result[1] == "===NVDA===");
}

TEST_CASE("base buy with leftover") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,NVDA,BUY,172.5,210");
    input.emplace_back("INSERT,2,NVDA,SELL,172.5,200");

    auto result = run(input);

    REQUIRE(result.size() == 3);
    CHECK(result[0] == "NVDA,172.5,200,2,1");
    CHECK(result[1] == "===NVDA===");
    CHECK(result[2] == "172.5,10,,");
}

TEST_CASE("base buy with sell leftover") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,NVDA,BUY,172.5,200");
    input.emplace_back("INSERT,2,NVDA,SELL,172.5,210");

    auto result = run(input);

    REQUIRE(result.size() == 3);
    CHECK(result[0] == "NVDA,172.5,200,2,1");
    CHECK(result[1] == "===NVDA===");
    CHECK(result[2] == ",,172.5,10");
}

TEST_CASE("no match price") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,AMD,BUY,150,15");
    input.emplace_back("INSERT,2,AMD,SELL,151,30");

    auto result = run(input);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "===AMD===");
    CHECK(result[1] == "150,15,151,30");
}

TEST_CASE("no match symbol") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,AMD,BUY,150,15");
    input.emplace_back("INSERT,2,NVDA,SELL,150,15");

    auto result = run(input);

    REQUIRE(result.size() == 4);
    CHECK(result[0] == "===AMD===");
    CHECK(result[1] == "150,15,,");
    CHECK(result[2] == "===NVDA===");
    CHECK(result[3] == ",,150,15");
}

TEST_CASE("insert same id") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,NVDA,BUY,172.5,200");
    input.emplace_back("INSERT,1,NVDA,SELL,172.5,200");

    try {
        auto result = run(input);    
    } catch(std::runtime_error const & err) {
        CHECK(err.what() == std::string("Error: Invalid INSERT command!"));
    } catch(...) {
        FAIL("Expected std::runtime_error");
    }    
}

TEST_CASE("recycle id") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,NVDA,BUY,150,20");
    input.emplace_back("INSERT,2,AMD,SELL,150,20");
    input.emplace_back("PULL,1");
    input.emplace_back("INSERT,1,AMD,BUY,150,20");

    auto result = run(input);

    REQUIRE(result.size() == 3);
    CHECK(result[0] == "AMD,150,20,2,1");
    CHECK(result[1] == "===AMD===");
    CHECK(result[2] == "===NVDA===");
}

TEST_CASE("invalid price") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,GOOG,BUY,92.1234,20");
    input.emplace_back("INSERT,2,GOOG,SELL,92.12333,20");
 
    try {
        auto result = run(input);    
    } catch(std::runtime_error const & err) {
        CHECK(err.what() == std::string("Error: Invalid INSERT command!"));
    } catch(...) {
        FAIL("Expected std::runtime_error");
    }
}

TEST_CASE("buy all") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,GOOG,SELL,92,1");
    input.emplace_back("INSERT,2,GOOG,SELL,92,1");
    input.emplace_back("INSERT,3,GOOG,SELL,92,1");
    input.emplace_back("INSERT,4,GOOG,SELL,92,1");
    input.emplace_back("INSERT,5,GOOG,SELL,92,1");
    input.emplace_back("INSERT,6,GOOG,BUY,92,5");
 
    auto result = run(input);

    REQUIRE(result.size() == 6);
    CHECK(result[0] == "GOOG,92,1,1,6");
    CHECK(result[1] == "GOOG,92,1,2,6");
    CHECK(result[2] == "GOOG,92,1,3,6");
    CHECK(result[3] == "GOOG,92,1,4,6");
    CHECK(result[4] == "GOOG,92,1,5,6");
    CHECK(result[5] == "===GOOG===");
}

TEST_CASE("sell all") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,GOOG,BUY,92,1");
    input.emplace_back("INSERT,2,GOOG,BUY,92,1");
    input.emplace_back("INSERT,3,GOOG,BUY,92,1");
    input.emplace_back("INSERT,4,GOOG,BUY,92,1");
    input.emplace_back("INSERT,5,GOOG,BUY,92,1");
    input.emplace_back("INSERT,6,GOOG,SELL,92,5");
 
    auto result = run(input);

    REQUIRE(result.size() == 6);
    CHECK(result[0] == "GOOG,92,1,6,1");
    CHECK(result[1] == "GOOG,92,1,6,2");
    CHECK(result[2] == "GOOG,92,1,6,3");
    CHECK(result[3] == "GOOG,92,1,6,4");
    CHECK(result[4] == "GOOG,92,1,6,5");
    CHECK(result[5] == "===GOOG===");
}

TEST_CASE("pull invalid id") {
    auto input = std::vector<std::string>();

    input.emplace_back("PULL,1");
 
    try {
        auto result = run(input);    
    } catch(std::runtime_error const & err) {
        CHECK(err.what() == std::string("Error: Cannot pull order #1"));
    } catch(...) {
        FAIL("Expected std::runtime_error");
    }
}

TEST_CASE("amend invalid id") {
    auto input = std::vector<std::string>();

    input.emplace_back("AMEND,1,5,5");
 
    try {
        auto result = run(input);    
    } catch(std::runtime_error const & err) {
        CHECK(err.what() == std::string("Error: Cannot amend order #1"));
    } catch(...) {
        FAIL("Expected std::runtime_error");
    };
}

TEST_CASE("empty input") {
    auto input = std::vector<std::string>();
 
    auto result = run(input);

    REQUIRE(result.size() == 0);
}

TEST_CASE("empty order") {
    auto input = std::vector<std::string>();
 
    input.emplace_back("");
 
    try {
        auto result = run(input);    
    } catch(std::runtime_error const & err) {
        CHECK(err.what() == std::string("Error: Invalid command!"));
    } catch(...) {
        FAIL("Expected std::runtime_error");
    };
}

TEST_CASE("amend sell") {
    auto input = std::vector<std::string>();

    input.emplace_back("INSERT,1,WEBB,BUY,45.95,5");
    input.emplace_back("INSERT,2,WEBB,SELL,46,10");
    input.emplace_back("AMEND,2,45,5");

    auto result = run(input);

    REQUIRE(result.size() == 2);
    CHECK(result[0] == "WEBB,45.95,5,2,1");
    CHECK(result[1] == "===WEBB===");
}