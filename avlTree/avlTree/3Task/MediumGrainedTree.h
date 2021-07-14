#pragma once
#include <cstdint>
#include <cstddef>
#include <shared_mutex>
#include "smart_pointer.h"

using smart_pointer::SmartPointer;
using std::shared_timed_mutex;
using std::unique_lock;
using std::shared_lock;

template<typename key_type, typename value_type>
class avl_tree
{
    
    class node
    {
        friend avl_tree;
        
        key_type _key;
        value_type _value;
        unsigned int _height;
        SmartPointer<node> _left;
        SmartPointer<node> _right;
        SmartPointer<node> _parent;
        bool deleted;
        shared_timed_mutex mut;
        
        node(key_type key, value_type value)
        {
            _value = value;
            _key = key;
            _height = 1;
            _left = _right = _parent = nullptr;
            deleted = false;
        }
        
        node() {}
    };
    
    using nodepntr = SmartPointer<node>;
    nodepntr _tree;
    size_t _size = 0;

    
    class Iterator
    {
        friend avl_tree;
        nodepntr _node;
        
    public:
        explicit Iterator(nodepntr node = nullptr)
        : _node(node)
        {}
        
        Iterator& operator=(const Iterator& rhs){
            
            _node = rhs._node;
            return *this;
        }
        
        bool operator==(const Iterator& rhs)
        {
            return _node == rhs._node;
            
        }
        
        bool operator!=(const Iterator& rhs)
        {
            return _node != rhs._node;
        }
        
        value_type& value()
        {
            return _node -> _value;
        }
        
        value_type& operator*()
        {
            return value();
        }
        
        key_type& key()
        {
            return _node -> _key;
        }
        
        Iterator& operator++()
        {
            if(!_node){
                return *this;
            }
            
            if (_node -> _right) {
                auto tmp = _node -> _right;
                _node = tmp;
                while (_node -> _left) {
                    _node = _node -> _left;
                }
            } else {
                
                nodepntr before;
                
                do {
                    before = _node;
                    _node = _node -> _parent;
                } while (_node && before == _node -> _right);
            }
            if (_node && _node -> deleted)
                operator++();
            return *this;
            
        }
        
        const Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        Iterator operator--()
        {

            if(!_node) {
                return *this;
            }
            
            if (_node -> _left) {
                auto tmp = _node -> _left;
                _node = tmp;
                
                while (_node -> _right)
                    _node = _node -> _right;
            }
            else {
                nodepntr before;
                do {
                    before = _node;
                    _node = _node -> _parent;
                } while (_node && (_node -> deleted || before == _node -> _left));
            }
            
            return *this;
        }
        
        const Iterator operator--(int)
        {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
    };
    
public:
    
    avl_tree() :
    _tree(new node(key_type(), value_type())),
    _size(0)
    {}
    
    avl_tree(avl_tree& tree): avl_tree()
    {
        auto iter = tree.begin();
        auto end = tree.end();
        while (iter != end) {
            insert(iter.key(), iter.value());
            iter++;
        }
    }
    
    Iterator begin()
    {
        return Iterator(findMin(_tree -> _left));
    }
    
    Iterator begin() const
    {
        return Iterator(findMin(_tree -> _left));
    }
    
    Iterator end()
    {
        return Iterator(nodepntr(nullptr));
    }
    
    Iterator end() const
    {
        return Iterator(nodepntr(nullptr));
    }
    
    size_t size()
    {

        return _size;
    }
    
    bool empty() const
    {
        return _size == static_cast<size_t>(0);
    }
    
    void clear()
    {
        _size = 0U;
        _tree -> _left = NULL;
    }
    
    Iterator insert(const key_type& key, const value_type& value)
    {
        auto it = find(key);
        if (it != end()) return it;
        return _insert_iterative( key, value);
    }
    
    bool remove(const key_type& key)
    {
        auto nd = _find(_tree -> _left, key);
        if (!nd) return false;
        remove(Iterator(nd));
        return true;
    }
    
    bool remove(Iterator pos) {
        return _remove_iterative(pos);
    }
    
    Iterator find(const key_type& key)
    {
        if (!_tree -> _left){
            return end();
        }
        return Iterator(_find(_tree -> _left, key));
    }
    
    value_type& operator[](const key_type& key)
    {
        auto res = insert(key, value_type());
        return res.value();
    }
    
    value_type& operator[](key_type&& key)
    {
        auto res = insert(std::move(key), value_type());
        return res.value();
    }
    
    ~avl_tree()
    {
        clear();
    }
    
private:
    
    unsigned int height(nodepntr _node)
    {
        return _node ? _node -> _height : 0;
    }
    
    int bfactor(nodepntr _node){
        return height(_node -> _right) - height(_node -> _left);
    }
    
    void updHeight (nodepntr _node)
    {
        if (_node){
            unsigned int hl = height(_node -> _left);
            unsigned int hr = height(_node -> _right);
            _node -> _height = (hr > hl ? hr : hl) + 1;
        }
    }
    
    nodepntr rightRotate(nodepntr _node)
    {
        nodepntr tmp = _node -> _left;
        _node -> _left = tmp -> _right;
        tmp -> _right = _node;
        updHeight(_node);
        updHeight(tmp);
        return tmp;
    }
    
    nodepntr leftRotate(nodepntr _node)
    {
        nodepntr tmp = _node -> _right;
        _node -> _right = tmp -> _left;
        tmp -> _left = _node;
        updHeight(_node);
        updHeight(tmp);
        return tmp;
    }
    
    nodepntr balance(nodepntr _node)
    {
        updHeight(_node);
        
        if (bfactor(_node) == 2){
            if (bfactor(_node -> _right) < 0){
                _node -> _right = rightRotate(_node -> _right);
            }
            return leftRotate(_node);
        }
        else if (bfactor(_node) == -2){
            if (bfactor(_node -> _left) > 0){
                _node -> _left = leftRotate(_node -> _left);
            }
            return rightRotate(_node);
        }
        else {
            return _node;
        }
    }
    
    nodepntr _insert(nodepntr _node, key_type key, value_type val)
    {
        if( !_node ){
            return nodepntr(new node(key, val));
        }
        
        else if( key < _node -> _key ) {
            _node -> _left = _insert( _node -> _left, key, val);
        }
        
        else if (key > _node -> _key ){
            _node -> _right = _insert( _node -> _right, key, val);
        }
        
        return balance(_node);
    }
    
    void set_left(nodepntr src, nodepntr n) {
        src -> _left = n;
        set_parent(src, n);
        updHeight(src);
    }
    
    void set_right(nodepntr src, nodepntr n) {
        src -> _right = n;
        set_parent(src, n);
        updHeight(src);
    }
    
    void set_parent(nodepntr src, nodepntr n) {
        if (n)
            n -> _parent = src;
    }
    
    Iterator _insert_iterative(const key_type& key, const value_type& value)
    {
        nodepntr nd (new node(key, value));
        _insert_node(nd);
        _balance_iterative(nd);
        _size++;
        set_parent(nodepntr(nullptr), _tree -> _left);
        return Iterator(_find(_tree -> _left, key));
    }
    
    void _insert_node(nodepntr nd) {
        unique_lock<shared_timed_mutex> node_lock (nd -> mut);
        auto parent = _tree -> _left;
        
        if (!parent) {
            unique_lock<shared_timed_mutex> ins_lock (_tree -> mut);
            _tree -> _left = nd;
            return;
        }
        
        while (parent) {
            unique_lock<shared_timed_mutex> ins_lock (parent -> mut);
            if (nd -> _key < parent -> _key) {
                if (!parent -> _left) {
                    set_left(parent, nd);
                    break;
                } else {
                    unique_lock<shared_timed_mutex> lock_tmp(parent -> _left -> mut);
                    parent = parent -> _left;
                }
            } else if (nd -> _key > parent -> _key) {
                if (!parent -> _right) {
                    set_right(parent, nd);
                    break;
                } else {
                    unique_lock<shared_timed_mutex> lock_tmp( parent -> _right -> mut);
                    parent = parent -> _right;
                }
            }
        }
    }
    
    void _balance_iterative(nodepntr nd) {
        nodepntr n = nd;
        
        while (n) {
            auto res = n;
            auto parent = n -> _parent;
            
            if (bfactor(n) == 2) {
                if (bfactor(n -> _right) < 0)
                    set_right(n, rightRotate(n -> _right));
                
                res = leftRotate(n);
            }
            if (bfactor(n) == -2) {
                if (bfactor(n -> _left) > 0)
                    set_left(n, leftRotate(n -> _left));
                
                res = rightRotate(n);
            }
            if (!parent) {
                _tree -> _left = res;
            } else {
                if (parent -> _left == n)
                    set_left(parent, res);
                else
                    set_right(parent, res);
            }
            
            n = parent;
        }
    }
    
    bool _remove_iterative(Iterator pos) {
        if (!pos._node || pos._node -> deleted)
            return false;
        
        auto res = _remove_node(pos._node);
        _balance_iterative(res);
        
        set_parent(nodepntr(nullptr), _tree -> _left);
        _size--;
        return true;
    }
    
    nodepntr _remove_node(nodepntr n) {
        auto to_delete = n;
        auto to_rebalance = nodepntr(nullptr);
        auto to_replace = n;
        auto to_replace_parent = nodepntr(nullptr);
        
        if (!to_delete -> _left && !to_delete -> _right && !to_delete -> _parent) {
            _tree -> _left = nodepntr(nullptr);
            to_delete -> deleted = true;
            return to_rebalance;
        }
        if (to_replace -> _right) {
            to_replace = to_replace -> _right;
            while (to_replace -> _left)
                to_replace = to_replace -> _left;
            
            to_replace_parent = to_replace -> _parent;
            
            
            set_parent(to_delete -> _parent, to_replace);
            set_left(to_replace, to_delete -> _left);
            if(to_delete -> _right != to_replace)
                set_right(to_replace, to_delete -> _right);
            
            
        }
        else {
            if(to_replace -> _left)
                to_replace = to_replace -> _left;
            else
                to_replace = nodepntr(nullptr);
        }
        
        
        
        if(to_replace && to_replace -> _parent != to_delete)
            to_rebalance = to_replace -> _parent;
        
        auto to_delete_parent = n -> _parent;
        
        if (to_delete_parent) {
            if (to_delete_parent -> _left == to_delete)
                set_left(to_delete_parent, to_replace);
            else
                set_right(to_delete_parent, to_replace);
            
            if (!to_delete -> _left && !to_delete -> _right)
                to_rebalance = to_delete_parent;
        }
        
        to_delete->deleted = true;
        if(to_replace && !to_replace -> _parent && !to_replace -> _left && !to_replace -> _right)
            _tree -> _left = to_replace;
        if(to_replace_parent && to_replace_parent != n) {
            if(to_replace_parent -> _left == to_replace)
                set_left(to_replace_parent, nodepntr(nullptr));
            else
                set_right(to_replace_parent, nodepntr(nullptr));
        }
        return to_rebalance;
    }
    nodepntr findMin(nodepntr _node)
    {
        return _node -> _left ? findMin(_node -> _left) : _node;
    }
    
    nodepntr findMin(nodepntr _node) const
    {
        return _node -> _left ? findMin(_node -> _left) : _node;
    }
    
    nodepntr findMax(nodepntr _node)
    {
        return _node -> _right ? findMax(_node -> _right) : _node;
    }
    
    nodepntr _find(nodepntr _node, const key_type& key)
    {
        if(_node -> _left && _node -> _key > key){
            return _find(_node -> _left, key);
        }
        
        else if (_node -> _right && _node -> _key < key){
            return _find(_node -> _right, key);
        }
        
        else if (_node -> _key != key){
            return nodepntr(nullptr);
            
        }
        else{
            return _node;
        }
        
    }
    
    nodepntr removeMin(nodepntr _node)
    {
        if( _node -> _left == 0 )
            return _node -> _right;
        
        _node -> _left = removeMin(_node -> _left);
        return balance(_node);
    }
    
    nodepntr _remove(nodepntr _node, key_type key)
    {
        
        if (!_node){
            return nodepntr(nullptr);
        }
        
        else if( key < _node -> _key ){
            _node -> _left = _remove( _node -> _left, key);
        }
        
        else if( key > _node -> _key ){
            _node -> _right = _remove( _node -> _right, key);
        }
        
        else
        {
            nodepntr q = _node -> _left;
            nodepntr r = _node -> _right;
            
            _node -> deleted = true;
            
            if (!r){
                return q;
            }
            
            nodepntr min = findMin(r);
            min -> _right = removeMin(r);
            min -> _left = q;
            return balance(min);
        }
        
        return balance(_node);
    }
};
