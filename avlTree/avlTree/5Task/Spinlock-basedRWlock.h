#pragma once

#include <iostream>
#include <thread>
#include <shared_mutex>
#include <cassert>

#include "RWSpinLock.h"

using std::shared_timed_mutex;
using std::unique_lock;
using std::shared_lock;

template <typename value_type>
class List
{
    template <typename T>
    class Node {
    private:
        template<typename U>
        friend class List;
        
    public:
        value_type data;
        Node* _left;
        Node* _right;
        List* list_pointer;
        std::atomic<std::uint32_t> ref_count = {0};
        std::atomic<std::uint32_t> bin = {0};
//        std::shared_timed_mutex mut;
        RWSpinLock mut;
        bool deleted;
        
        Node(List* list) : _left(nullptr), _right(nullptr), list_pointer(list), deleted(false)
        {}
        
        Node(value_type data, List* list) : data(data), _left(nullptr), _right(nullptr), list_pointer(list), deleted(false)
        {}
        
        static void receive(Node** dst, Node* node) {
            *dst = node;
            node -> ref_count++;
        }
        
        void release(Node* node) {
            
            list_pointer -> list_mut.rlock();
            
            
            std::uint32_t old_ref_count = --node -> ref_count;
            
            if (old_ref_count == 0 && !deleted)
            {
                list_pointer -> bin -> deleteNode(node);
            }
            
            list_pointer -> list_mut.unlock();
            
        }
        
    };
    
    template <typename T>
    class Bin {
    private:
        template<typename U>
        friend class List;
        
    public:
        using Nod = Node<value_type>;
        
        template <typename S>
        class BinNode {
        public:
            BinNode(Nod* data) : _data(data)
            {}
            
            Nod* _data = nullptr;
            BinNode* _right = nullptr;
        };
        
        using binNode = BinNode<value_type>;
        
        std::thread personalThread;
        std::atomic<binNode*> start;
        List* list_pointer;
        bool terminated = false;
        
        Bin(List* list) : list_pointer(list), start(nullptr), personalThread(&Bin::sleepPThread, this)
        {}
        
        ~Bin() {
            terminated = true;
            personalThread.join();
        }
        
        void freeNode(binNode* node) {
            Nod* left = node -> _data -> _left;
            Nod* right = node -> _data -> _right;
            
            if (left != nullptr)
                left -> release(left);
            
            if (right != nullptr)
                right -> release(right);
            
            delete node -> _data;
            delete node;
        }
        
        void remove (binNode* left, binNode* node) {
            left -> _right = node -> _right;
            delete node;
        }
        
        void deleteNode(Nod* data) {
            auto node = new binNode(data);
            
            do {
                node -> _right = start.load();
            } while (!start.compare_exchange_strong(node -> _right, node));
        }
        
        
        void sleepPThread() {
            do {
                // первая стадия
                list_pointer -> list_mut.wlock();
                
                binNode* binStart = this -> start;
                
                list_pointer -> list_mut.unlock();
                
                if (binStart != nullptr) {
                    
                    binNode* left = binStart;
                    for (binNode* node = binStart; node != nullptr;) {
                        
                        binNode* tmp = node;
                        node = node -> _right;
                        
                        if (tmp -> _data -> ref_count > 0 || tmp -> _data -> deleted) {
                            remove(left, tmp);
                        } else {
                            tmp -> _data -> bin = 1;
                            left = tmp;
                        }
                    }
                    
                    left -> _right = nullptr;
                    
                    // вторая стадия
                    
                    list_pointer -> list_mut.wlock();
                    
                    binNode* newBinStart = this -> start;
                    
                    if (newBinStart == binStart) {
                        // если после первой стадии нет новых нодов, то start освобождается
                        
                        start = nullptr;
                    }
                    
                    list_pointer -> list_mut.unlock();
                    
                    left = newBinStart;
                    binNode* node = newBinStart;
                    while (node != binStart) {
                        binNode* tmp = node;
                        node = node -> _right;
                        
                        if (tmp -> _data -> bin == 1) {
                            remove(left, tmp);
                        } else {
                            left = tmp;
                        }
                    }
                    left -> _right = nullptr;
                    
                    
                    for (binNode* node = binStart; node != nullptr; ) {
                        binNode* tmp = node;
                        node = node -> _right;
                        
                        freeNode(tmp);
                    }
                }
                
                if (!terminated) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } while (!terminated || start.load() != nullptr);
        }
        
    };
    
public:
    template <typename T>
    class Iterator
    {
    private:
        template<typename U>
        friend class List;
    public:
        using Nod = Node<value_type>;
        
        Nod* pointer;
        List* list_pointer;
        
        Iterator(Nod* ptr, List* list_ref) : pointer(ptr), list_pointer(list_ref) {
            ptr -> ref_count++;
        }
        
        Iterator(const Iterator& sp) : pointer(sp.pointer), list_pointer(sp.list_pointer) {
            pointer -> ref_count++;
        }
        
        Iterator(Iterator&& sp) : pointer(sp.pointer), list_pointer(sp.list_pointer) {
            pointer -> ref_count++;
        }
        
        Iterator& operator =(const Iterator& sp) {
            if (pointer == sp.pointer)
                return *this;
            
            Nod* old_ptr = pointer;
            
            pointer = sp.pointer;
            list_pointer = sp.list_pointer;
            
            if (old_ptr != nullptr)
                pointer -> release(pointer);
            
            return *this;
        }
        
        Iterator& operator =(Iterator&& sp) {
            if (pointer == sp.pointer)
                return *this;
            
            Nod* old_ptr = pointer;
            
            pointer = sp.pointer;
            list_pointer = sp.list_pointer;
            
            if (old_ptr != nullptr)
                pointer -> release(pointer);
            
            return *this;
        }
        
        ~Iterator() {
            pointer -> release(pointer);
        }
        
        value_type operator*() const {
            return pointer -> data;
        }
        
        Nod* operator->() const {
            return pointer;
        }
        
        Iterator& operator++() {
            Nod* node = pointer;
            
            list_pointer -> list_mut.reedLock();
            
            auto next_node = node -> next;
            
            value_type::acquire(&pointer, next_node);
            
            list_pointer -> list_mut.unlock();
            
            node -> release(node);
            
            return *this;
        }
        
        Iterator operator++(int) {
            ++(*this);
            return *this;
        }
        
        Iterator& operator--() {
            auto node = pointer;
            list_pointer -> list_mut.rlock();
            
            auto prev_node = node -> _left;
            
            Nod::receive(&pointer, prev_node);
            
            list_pointer -> list_mut.unlock();
            
            node -> release(node);
            
            return *this;
        }
        
        Iterator operator--(int) {
            --(*this);
            return *this;
        }
        
        bool operator== (const Iterator& other) {
            return pointer == other.pointer;
        }
        
        bool operator!= (const Iterator& other) {
            return !(*this == other);
        }
        
    };
    
    using Nod = Node<value_type>;
    using Iter = Iterator<value_type>;
    
    List() : first(new Nod(this)), last(new Nod(this)), bin(new Bin<value_type>(this)) {
        first -> ref_count = 1;
        first -> _left = nullptr;
        first -> _right = nullptr;
        
        last -> ref_count = 1;
        last -> _left = nullptr;
        last -> _right = nullptr;
        
        _size = 0;
        
        Nod::receive(&first -> _right, last);
        Nod::receive(&last -> _left, first);
    }
    
    List(value_type data) : first(new Nod(this)), last(new Nod(this)), bin(new Bin<value_type>(this)) {
        first -> ref_count = 1;
        first -> _left = nullptr;
        first -> _left = nullptr;
        
        last -> ref_count = 1;
        last -> _left = nullptr;
        last -> _right = nullptr;
        
        _size = 0;
        
        Nod::receive(&first -> _right, last);
        Nod::receive(&last -> _left, first);
        
        push_back(data);
    }
    
    ~List() {
        delete(bin);
        
        auto node = first -> _right;
        while (node != last) {
            auto buf = node;
            
            node = node -> _right;
            
            delete buf;
        }
        
        delete first;
        delete last;
    }
    
    Iter begin() {
//        shared_lock<shared_timed_mutex> lock(first -> mut);
        first -> mut.rlock();
        Nod* ret = first -> _right;
        Iter it = Iter(ret, this);
        
        return it;
    }
    
    Iter end() {
        return Iter(last, this);
    }
    
    Iter insert(Iter& iter, value_type data) {
        Nod* prev = iter.pointer;
        
        if (iter.pointer == last) {
            throw std::out_of_range("out of range");
        }
        
        for (bool retry = true; retry; ) {
            if (iter.pointer -> deleted) {
                // if node is deleted we will not insert el after it return end()
                return Iter(last, this);
            }
            
//            unique_lock<shared_timed_mutex> lock_prev(prev->mut);
            prev -> mut.wlock();
            Nod* next = prev-> _right;;
            next -> mut.wlock();
//            unique_lock<shared_timed_mutex> lock_next(next->mut);
            
            if (prev == next -> _left) {
                Nod* new_node = new Nod(data, this);
                new_node -> mut.wlock();
//                unique_lock<shared_timed_mutex> lock_new(new_node->mut);
                
                Nod::receive(&new_node -> _left, prev);
                Nod::receive(&new_node-> _right, next);
                
                Nod::receive(&prev-> _right, new_node);
                Nod::receive(&next -> _left, new_node);
                
                prev->release(prev); // ref_count >= 3
                next->release(next);
                
                _size++;
                retry = false;
                
                // iter++
                Nod::receive(&iter.pointer, new_node);
                
                new_node -> mut.unlock();
            }
            
            next -> mut.unlock();
            prev -> mut.unlock();
        }
        
        prev -> release(prev);
        return iter;
    }
    
    Iter erase(Iter& iter) {
        Nod* node = iter.pointer;
        
        for (bool retry = true; retry; ) {
//            shared_lock<shared_timed_mutex> lock_node(node -> mut);
            node -> mut.rlock();
            if (node == last || node == first) {
                throw std::out_of_range("out of range");
            }
            
            Nod* prev = node -> _left;
            prev->ref_count++;
            
            Nod* next = node-> _right;
            next->ref_count++;
            
            node -> mut.unlock();
            
//            unique_lock<shared_timed_mutex> lock_prev(prev->mut);
            prev -> mut.wlock();
            node -> mut.rlock();
            next -> mut.wlock();
//            unique_lock<shared_timed_mutex> lock_next(next->mut);
            
            if (node-> deleted) {
                // iter++
                Nod::receive(&iter.pointer, next);
                
                next -> mut.unlock();
                node -> mut.unlock();
                prev -> mut.unlock();
                
                prev->release(prev);
                next->release(next);
                
                // ref_cnt cause iter++
                node->release(node);
                return iter;
            }
            
            if (prev-> _right == node && next -> _left == node) {
                Nod::receive(&prev -> _right, next);
                Nod::receive(&next -> _left, prev);
                
                node -> deleted = true;
                
                node -> release(node);
                node -> release(node);
                
                _size--;
                
                retry = false;
                
                // iter++
                Nod::receive(&iter.pointer, next);
            }
            
            next -> mut.unlock();
            node -> mut.unlock();
            prev -> mut.unlock();
            
            prev -> release(prev);
            next -> release(next);
        }
        
        node->release(node);
        return iter;
    }
    
    void push_front(value_type data) {
        auto it = Iter(first, this);
        insert(it, data);
    }
    
    void push_back(value_type data) {
        auto it = Iter(last -> _left, this);
        insert(it, data);
    }
    
    bool empty() {
        return !_size;
    }
    
    unsigned long size() {
        return _size;
    }
    
private:
    Nod* first;
    Nod* last;
    std::atomic<std::size_t> _size = {0};
    RWSpinLock list_mut;
    Bin<value_type>* bin;
};
