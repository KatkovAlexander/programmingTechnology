#include "avl_tree.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <string>

using std::string;
using std::rand;

TEST_CASE("basic avl tests") {
    avl_tree<char, string> tree;
    tree.insert('0', "abc");
    tree.insert('1', "zxc");
    tree.insert('2', "klj");

    avl_tree<char, string> copy(tree);

    REQUIRE(copy['0'] == "abc");
    REQUIRE(tree.size() == copy.size());

    REQUIRE(tree.find('2').val() == "klj");
    REQUIRE(tree.size() == 3);

    tree.clear();
    REQUIRE(tree.find('0') == tree.end());
    REQUIRE(tree.find('1') == tree.end());
    REQUIRE(tree.find('2') == tree.end());
    REQUIRE(tree.size() == 0);
    REQUIRE(tree.empty());


    REQUIRE(tree.begin() == tree.end());
    tree['y'] = "z";
    REQUIRE(tree.find('y') != tree.end());
}

//TEST_CASE("Consistency") {
//    auto tree = avl_tree<int, int>();
//    tree.insert(1, 2);
//    tree.insert(3, 4);
//    tree.insert(5, 6);
//
//    auto it = tree.begin();
//
//    ++it;
//
//    REQUIRE(it.key() == 3);
//    REQUIRE(it.val() == 4);
//
//    tree.erase(3);
//
//    ++it;
//
//    REQUIRE(it.key() == 5);
//    REQUIRE(it.val() == 6);
//}

//TEST_CASE("Consistency_correct_end") {
//    int n = 5000;
//    auto tree = avl_tree<int, int>();
//    for (int i = 0; i < n; ++i) {
//        auto key = rand() % 5000;
//        auto value = rand();
//        tree.insert(key, value);
//    }
//
//    auto its = std::vector<avl_tree<int, int>::iterator>();
//    for (int i = 0; i < n; ++i) {
//        auto it = tree.begin();
//        auto steps = rand() % 2000;
//        for (int j = 1; j < steps; ++j) {
//            ++it;
//            if (it == tree.end()) break;
//        }
//        its.push_back(it);
//    }
//
//    for (int i = 0; i < n; ++i) {
//        auto key = rand() % 5000;
//        tree.erase(key);
//    }
//
//    for (auto it : its) {
//        while (it != tree.end()) {
//            ++it;
//        }
//    }
//}