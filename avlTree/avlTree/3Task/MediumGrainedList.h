#pragma once

#include <iostream>
#include <cstddef>
#include <shared_mutex>

using std::shared_timed_mutex;
using std::unique_lock;
using std::shared_lock;

template<typename value_type>
class List {
    
    typedef class Node
    {
    public:
        value_type _value;
        Node* _left;
        Node* _right;
        std::atomic<std::uint32_t> ref_count = {0};
        shared_timed_mutex mut;
        bool deleted;
        
        Node(value_type value) : _value(value), _left(nullptr), _right(nullptr), deleted(false) {}
        
        Node() : _left(nullptr), _right(nullptr), deleted(false) {}
        
        static void deleteNode(Node* node)
        {
            if (node -> _left != nullptr) {
                release(node -> _left);
            }
            if (node -> _right != nullptr) {
                release(node -> _right);
            }

            delete node;
        }
        
        static void receive(Node** dst, Node* node)
        {
            *dst = node;
            node -> ref_count++;
        }
        
        static void release(Node* node){
            node -> ref_count--;
            
            if (node -> ref_count == 0) {
                deleteNode(node);
            }
        }
        
    } Node;
    
    
    typedef class Iterator
    {
    public:
        
        Node* pointer;
        
        Iterator (Node* pntr) : pointer(pntr) {
            pointer -> ref_count++;
        }
        
        Iterator(const Iterator& sp) : pointer(sp.pointer) {
            pointer -> ref_count++;
        }
        
        Iterator(Iterator&& sp) : pointer(sp.pointer) {
            pointer -> ref_count++;
        }
        
        Iterator& operator= (const Iterator& sp){
            unique_lock<shared_timed_mutex> lock(pointer->mut);{
                unique_lock<shared_timed_mutex> lock(sp->mut);
                {}
            }
            
            if (pointer == sp.pointer) { return *this; }
            
            if (sp.pointer) {
                sp.pointer -> ref_count++;
            }
            
            Node::release(pointer);
            pointer = sp.pointer;
            return *this;
        }
        
        Iterator& operator= (Iterator&& sp){
            unique_lock<shared_timed_mutex> lock(pointer->mut);{
                unique_lock<shared_timed_mutex> lock(sp->mut);
                {}
            }
            
            if (pointer == sp.pointer) { return *this; }
            
            if (sp.pointer) {
                sp.pointer -> ref_count++;
            }
            
            Node::release(pointer);
            pointer = sp.pointer;
            return *this;
        }
        
        ~Iterator() {
            unique_lock<shared_timed_mutex> lock(pointer->mut);
            Node::release(pointer);
        }
        
        Iterator& operator++() {
            Node* node = pointer;
            shared_lock<shared_timed_mutex> lock (node -> mut);
            auto next = node -> _right;
            Node::receive(&pointer, next);
            
            if (node -> ref_count == 1) {
                lock.unlock();
                unique_lock<shared_timed_mutex> lock(node->mut);
                Node::release(node);
            }
            else {
                Node::release(node);
            }
            
            return *this;
        }
        
        Iterator operator++ (int) {
            ++(*this);
            return *this;
        }
        Iterator& operator-- () {
            Node* node = pointer;
            shared_lock<shared_timed_mutex> lock (node -> mut);
            auto prev = node -> _left;
            Node::receive(&pointer, prev);
            
            if (node -> ref_count == 1) {
                lock.unlock();
                unique_lock<shared_timed_mutex> lock(node->mut);
                Node::release(node);
            }
            else {
                Node::release(node);
            }
            
            return *this;
        }
        
        Iterator operator-- (int) {
            --(*this);
            return *this;
        }
        
        value_type operator*() {
            return pointer->_value;
        }
        
        value_type operator*() const {
            return pointer->_value;
        }
        
        Node* operator->() const {
            return pointer;
        }
        
        bool operator== (const Iterator& sp) {
            return pointer == sp.pointer;
        }
        
        bool operator!= (const Iterator& sp) {
            return !(pointer == sp.pointer);
        }
        
    } Iterator;
    
    friend Iterator;
    
public:
    List() : first(new Node), last(new Node) {
        first -> ref_count = 1;
        first -> _left = nullptr;
        first -> _right = nullptr;
        
        last -> ref_count = 1;
        last -> _left = nullptr;
        last -> _right = nullptr;
        
        _size = 0;
        
        Node::receive(&first->_right, last);
        Node::receive(&last->_left, first);
    }
    
    List (value_type value) : first(new Node), last(new Node) {
        first -> ref_count = 1;
        first -> _left = nullptr;
        first -> _right = nullptr;
        
        last -> ref_count = 1;
        last -> _left = nullptr;
        last -> _right = nullptr;
        
        _size = 0;
        
        Node::receive(&first->_right, last);
        Node::receive(&last->_left, first);
        
        push_back(value);
    }
    
    ~List() {
        auto next = first -> _right;
        while (next != last){
            auto tmp = next;
            delete tmp;
            next = next -> _right;
        }
        
        delete first;
        delete last;
    }
    
    Iterator begin () {
        shared_lock<shared_timed_mutex> lock (first -> mut);
        Node* ret = first -> _right;
        Iterator it = Iterator(ret);
        return it;
    }
    
    Iterator end () {
        shared_lock<shared_timed_mutex> lock (first -> mut);
        return Iterator(last);
    }
    
    Iterator insert(Iterator& it, value_type value) {
        auto previous = it.pointer;
        bool added = false;
        
        if (it.pointer == last) {
            throw std::out_of_range("out of range");
        }
        
        while (!added) {
            if (it.pointer -> deleted) {
                return Iterator(last);
            }
            
            unique_lock<shared_timed_mutex> lock1 (previous -> mut);
            auto next = previous -> _right;
            unique_lock<shared_timed_mutex> lock2 (next -> mut);
            
            if (next->_left == previous) {
                Node* newNode = new Node(value);
                unique_lock<shared_timed_mutex> lock(newNode -> mut);
                
                _size++;
                
                Node::receive(&previous -> _right, newNode);
                Node::receive(&newNode -> _left, previous);
                
                Node::receive(&next -> _left, newNode);
                Node::receive(&newNode -> _right, next);
                
                Node::release(previous);
                Node::release(next);
                
                Node::receive(&it.pointer, newNode);
                Node::release(previous);
                
                added = true;
            }
        }
        
        return it;
    }
    
    Iterator erase(Iterator& it) {
        auto node = it.pointer;
        bool deleted = false;
        
        while (!deleted) {
            shared_lock<shared_timed_mutex> lock1 (node -> mut);
            
            if (node == first || node == last) {
                throw std::out_of_range("out of range");
            }
            
            auto previous = node -> _left;
            previous -> ref_count++;
            
            auto next = node -> _right;
            next -> ref_count++;
            
            lock1.unlock();
            
            unique_lock<shared_timed_mutex> lock2 (previous -> mut);
            lock1.lock();
            unique_lock<shared_timed_mutex> lock3 (next -> mut);
            
            if (node -> deleted) {
                Node::receive(&it.pointer, next);
                Node::release(node);
                
                Node::release(previous);
                Node::release(next);
                
                return it;
            }
            
            if (previous -> _right == node && next -> _left == node) {
                Node::receive(&previous -> _right, next);
                Node::receive(&next -> _left, previous);
                node -> deleted = true;
                _size--;
                
                deleted = true;
                
                Node::receive(&it.pointer, next);
            }
            
            Node::release(previous);
            Node::release(next);
        }
        
        return it;
    }
    
    void push_front(value_type value) {
        auto it = Iterator(first);
        insert(it, value);
    }
    
    void push_back(value_type value) {
        auto it  = Iterator(last->_left);
        insert(it, value);
    }
    
    bool empty() {
        return !_size;
    }
    
    int size() {
        return _size;
    }
    
private:
    Node* first;
    Node* last;
    std::atomic<std::uint32_t> _size = {0};
};
