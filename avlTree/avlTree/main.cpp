#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "test_runner.h"
#include "avl_tree.h"

using namespace std;

void TestIterEq()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    auto iter = tree.begin();
    iter.value() = "123";
    ASSERT_EQUAL(*iter, "123");
}

void TestIterDblEq()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    
    avl_tree<int, string> copy(tree);
    ASSERT_EQUAL(*tree.begin() == *copy.begin(), 1);
    
}

void TestIterPP()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    auto iter = tree.begin();
    iter++;
    ASSERT_EQUAL(iter.value(), "123");
}

void TestIterMM()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    auto iter = tree.begin();
    iter++;
    iter++;
    iter--;
    ASSERT_EQUAL(iter.value(), "123");
}

void TestAvlInsertAndEmpty()
{
    avl_tree<int, string> tree;
    ASSERT_EQUAL(tree.empty(), true);
    
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    ASSERT_EQUAL(tree.empty(), false)
}

void TestAvlClear()
{
    avl_tree<int, string> tree;
    ASSERT_EQUAL(tree.empty(), true);
    
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    ASSERT_EQUAL(tree.empty(), false);
    tree.clear();
    ASSERT_EQUAL(tree.empty(), true);
}

void TestAvlRemove()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    ASSERT_EQUAL(tree.empty(), false);
    tree.remove(0);
    ASSERT_EQUAL(tree.empty(), true);
}

void TestAvlBegin()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    ASSERT_EQUAL(*tree.begin(), "12");
}

void TestAvlFind()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    ASSERT_EQUAL(*tree.find(0), "12");
}

void TestAvlEnd()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    auto iter = tree.begin();
    ostringstream os;
    while (iter != tree.end()) {
        os << *iter;
        iter++;
    }
    
    ASSERT_EQUAL(os.str(), "121231234");
}

void TestKeyAndValFunc()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    
    auto iter  = tree.begin();
    ASSERT_EQUAL(iter.key(), 0);
    ASSERT_EQUAL(iter.value(), "12");
}

void TestAvlSquareBrackets()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    ASSERT_EQUAL(tree[1], "123");
}

void ConsitencyOnlyPP()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    auto iter = tree.begin();
    iter++;
    
    ASSERT_EQUAL(iter.key(), 1)
    ASSERT_EQUAL(*iter, "123");
    
    tree.remove(1);
    iter++;
    
    ASSERT_EQUAL(iter.key(), 2);
    ASSERT_EQUAL(*iter, "1234");
    
    
}

void ConsitencyPPPPMM()
{
    avl_tree<int, string> tree;
    tree.insert(0, "12");
    tree.insert(1, "123");
    tree.insert(2, "1234");
    
    auto iter = tree.begin();
    iter++;
    
    ASSERT_EQUAL(iter.key(), 1)
    ASSERT_EQUAL(*iter, "123");
    
    tree.remove(1);
    iter++;
    
    ASSERT_EQUAL(iter.key(), 2);
    ASSERT_EQUAL(*iter, "1234");
    
    iter--;
    
    ASSERT_EQUAL(iter.key(), 0)
    ASSERT_EQUAL(*iter, "12");
    
}

void ConsistencyTests(){
    TestRunner tr;
    
    RUN_TEST(tr, TestIterEq);
    RUN_TEST(tr, TestIterDblEq);
    RUN_TEST(tr, TestIterPP);
    RUN_TEST(tr, TestIterMM);
    RUN_TEST(tr, TestAvlInsertAndEmpty);
    RUN_TEST(tr, TestAvlClear);
    RUN_TEST(tr, TestAvlRemove);
    RUN_TEST(tr, TestAvlBegin);
    RUN_TEST(tr, TestAvlFind);
    RUN_TEST(tr, TestAvlEnd);
    RUN_TEST(tr, TestAvlSquareBrackets);
    RUN_TEST(tr, TestKeyAndValFunc);
    RUN_TEST(tr, ConsitencyOnlyPP);
    RUN_TEST(tr, ConsitencyPPPPMM);
}

void AtomacityTest1()
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

void AtomicityTests()
{
    TestRunner tr;
    RUN_TEST(tr, AtomacityTest1);
}

int main(){
    
    ConsistencyTests();
    AtomicityTests();
    
    return 0;
}
