#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "test_runner.h"
#include "avl_tree.h"

using namespace std;


void AtomacityTestAdd()
{
    int n = 5;
    avl_tree<char, char> tree;
    vector<thread> ths(n);
    string s = "12345";

    string::iterator start = s.begin();
    
    auto lmbd = [&tree](string::iterator begin, string::iterator end)
    {
        for_each(begin, end, [&tree](char& c)
        {
            tree[c] = c;
        });
    };

    for (int i = 0; i < n; i++){
        string::iterator end = start;
        advance(end, 1);
        ths[i] = thread(lmbd, start, end);
        start = end;
    }

    for (auto& i : ths) {
        i.join();
    }
    
    auto it = tree.begin();
    ASSERT_EQUAL(*it, '1');
    it++;
    ASSERT_EQUAL(*it, '2');
    it++;
    ASSERT_EQUAL(*it, '3');
    it++;
    ASSERT_EQUAL(*it, '4');
    it++;
    ASSERT_EQUAL(*it, '5');
}

void AtomacityTestErase()
{
    int n = 10;
    avl_tree<char, char> tree;
    vector<thread> ths(n);
    string s = "0123456789";

    string::iterator start = s.begin();
    
    auto lmbd = [&tree](string::iterator begin, string::iterator end)
    {
        for_each(begin, end, [&tree](char& c)
        {
            tree[c] = c;
        });
    };

    for (int i = 0; i < n; i++){
        string::iterator end = start;
        advance(end, 1);
        ths[i] = thread(lmbd, start, end);
        start = end;
    }

    for (auto& i : ths) {
        i.join();
    }
    
    start = s.begin();
    
    auto lmbd2 = [&tree](string::iterator begin, string::iterator end)
    {
        for_each(begin, end, [&tree](char& c)
        {
            tree.remove(c);
        });
    };
    
    for (int i = 0; i < n; i++){
        string::iterator end = start;
        advance(end, 1);
        ths[i] = thread(lmbd2, start, end);
        start = end;
    }

    for (auto& i : ths) {
        i.join();
    }
    
    ASSERT_EQUAL(tree.empty(), true);
}

void Test()
{
    TestRunner tr;
    RUN_TEST(tr, AtomacityTestAdd);
    RUN_TEST(tr, AtomacityTestErase);
}
