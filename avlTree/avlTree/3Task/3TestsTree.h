#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "test_runner.h"
#include "MediumGrainedTree.h"

using namespace std;


void MediumGrainedTestAdd()
{
    avl_tree<char, char> tree;
    vector<thread> threads;
    int thNum = 4;
    int elNum = 1000;
    
    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thNum / 2; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                tree.insert(i * j * thNum, i * j * thNum);
            }
        }, i));
    }

    for (auto& i : threads) {
        i.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    cout << (double)time.count() / 1000.0 << endl;
    ASSERT_EQUAL(tree.size(), 2000);

}

void MediumGrainedTestRemove()
{
    avl_tree<char, char> tree;
    vector<thread> threads;
    int thNum = 4;
    int elNum = 1000;
    
    for (int i = 0; i < thNum / 2; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                tree.insert(i * j * thNum, i * j * thNum);
            }
        }, i));
    }

    for (auto& i : threads) {
        i.join();
    }
    
    auto startTime = chrono::high_resolution_clock::now();
    
    
    for (int i = 0; i < thNum; i++){
        threads[i] = thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                tree.remove(i * j * thNum);
            }
        }, i);
    }

    for (auto& i : threads) {
        i.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    cout << (double)time.count() / 1000.0 << endl;

}

void MediumGrainedTestAddAndRemove() {
    
    avl_tree<char, char> tree;
    vector<thread> threads;
    int thNum = 4;
    int elNum = 1000;
    
    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thNum / 2; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                tree.insert(i * j * thNum, i * j * thNum);
            }
        }, i));
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                tree.remove(i * j * thNum);
            }
        }, i));
    }

    
    for (auto& i : threads) {
        i.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    cout << (double)time.count() / 1000.0 << endl;
    ASSERT_EQUAL(tree.empty(), true);
}

void Test()
{
    TestRunner tr;
    RUN_TEST(tr, MediumGrainedTestAdd);
    RUN_TEST(tr, MediumGrainedTestRemove);
    RUN_TEST(tr, MediumGrainedTestAddAndRemove);
}
