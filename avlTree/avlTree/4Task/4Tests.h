#pragma once

#include <thread>
#include <vector>
#include <chrono>

#include "test_runner.h"
#include "FinegrainingList.h"

using namespace std;

void Insert() {
    List<int> list(1);
    ASSERT_EQUAL(list.size(), 1);
    
    list.push_back(2);
    list.push_back(3);
    list.push_front(9);
    
    list.push_front(3);
    ASSERT_EQUAL(list.size(), 5);
}

void Erase() {
    List<int> list(1);
    
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);
    
    auto it = list.end();
    it--;
    list.erase(it);
    it--;
    ASSERT_EQUAL(*it, 3);

}

void Equal() {
    List<int> list(1);
    
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);
    
    auto bg = list.begin();
    auto end1 = list.end();
    ASSERT_EQUAL(bg != end1, true);
    ASSERT_EQUAL(bg == end1, false);
    auto end2 = list.end();
    ASSERT_EQUAL(end1 == end2, true);
    ASSERT_EQUAL(end1 != end2, false);
}

void FineGrainedTestInsert() {
    List<int> list;
    vector<thread>  threads;
    int thNum = 4;
    int elNum = 1000;

    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thNum; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                list.push_back(j*i);
            }
        }, i));
    }
    
    
    for (auto& i : threads) {
        i.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    cout << (double)time.count() / 1000.0 << endl;
    ASSERT_EQUAL(list.size() == thNum * elNum, true);
}

void FineGrainedTestDelete() {
    List<int> list;
    vector<thread>  threads;
    int thNum = 4;
    int elNum = 1000;

    for (int i = 0; i < thNum; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                list.push_back(j*i);
            }
        }, i));
    }
    
    
    for (auto& i : threads) {
        i.join();
    }
    
    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thNum; i++) {
        threads[i] = thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                auto iter = list.end(); iter--;
                list.erase(iter);
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

void FineGrainedTestInsertUndDelete(){
    List<int> list;
    vector<thread>  threads;
    int thNum = 4;
    int elNum = 1000;

    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < thNum / 2; i++) {
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum; j++) {
                list.push_back(j*i);
            }
        }, i));
        threads.push_back(thread( [&] (int th) {
            for (int j = 0; j < elNum / 2; j++) {
                auto iter = list.end(); iter--;
                list.erase(iter);
            }
        }, i));
    }
    
    
    for (auto& i : threads) {
        i.join();
    }
    auto endTime = chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
    cout << (double)time.count() / 1000.0 << endl;

}

void Test() {
    TestRunner tr;
    RUN_TEST(tr, Insert);
    RUN_TEST(tr, Erase);
    RUN_TEST(tr, Equal);
    RUN_TEST(tr, FineGrainedTestInsert);
    RUN_TEST(tr, FineGrainedTestDelete);
    RUN_TEST(tr, FineGrainedTestInsertUndDelete);
}
