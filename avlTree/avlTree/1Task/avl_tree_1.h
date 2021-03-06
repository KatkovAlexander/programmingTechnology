#pragma once
#include <cstddef>
#include "smart_pointer.h"

using smart_pointer::SmartPointer;

template<typename key_type, typename value_type>
class avl_tree
{
    
    typedef struct node
    {
        key_type _key;
        value_type _value;
        unsigned int _height;
        SmartPointer<node> _left;
        SmartPointer<node> _right;
        bool deleted;
        
        node(key_type key, value_type value)
        {
            _value = value;
            _key = key;
            _height = 1;
            _left = _right = nullptr;
            deleted = false;
        }
    } node;
    
    using nodepntr = SmartPointer<node>;
    nodepntr _tree;
    size_t _size;
    
    typedef class Iterator
    {
        nodepntr _node;
        nodepntr _tree;
        
    public:
        explicit Iterator(nodepntr node = nullptr, nodepntr tree = nullptr)
        : _node(node), _tree(tree)
        {}
        
        Iterator& operator=(const Iterator& rhs){
            _node = rhs._node;
            return *this;
        }
        
        bool operator==(const Iterator& rhs)
        {
            return _node == !rhs._node;
            
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
        
        Iterator operator++()
        {
            if(!_node){
                return *this;
            }
            
            if (!_node -> deleted && _node -> _right) {
                _node = _node -> _right;
                
                while (_node -> _left)
                    _node = _node -> _left;
                return *this;
                
            } else {
                
                nodepntr q = _tree-> _left; 
                nodepntr l;
                
                while (q) {
                    if (q -> _key > _node -> _key) {
                        l = q;
                        q = q -> _left;
                        
                    } else if (q -> _key < _node -> _key)
                        q = q -> _right;
                    
                    else
                        break;
                }
                
                _node = l;
                return *this;
            }
        }
        
        Iterator operator++(int)
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
            
            if (!_node -> deleted && _node -> _left) {
                _node =  _node -> _left;
                while (_node -> _right)
                    _node = _node -> _right;
                return *this;
            }
            else {
                nodepntr q = _tree -> _left;
                nodepntr l;
                
                while (q) {
                    if (q -> _key < _node -> _key) {
                        l = q;
                        q = q -> _right;
                    }
                    else if (q -> _key > _node -> _key)
                        q = q -> _left;
                    else{
                        break;
                    }
                }
                
                _node = l;
                return *this;
            }
        }
        
        Iterator operator--(int)
        {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
    } Iterator;
    
    friend Iterator;
    
public:
    
    typedef Iterator iterator;
    avl_tree() :
    _tree(new node(key_type(), value_type())),
    _size(0)
    {}
    
    avl_tree(const avl_tree& tree): avl_tree()
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
        return Iterator(findMin(_tree -> _left), _tree);
    }
    
    Iterator begin() const
    {
        return Iterator(findMin(_tree -> _left), _tree);
    }
    
    Iterator end()
    {
        return Iterator(nodepntr(nullptr), _tree -> _left);
    }
    
    Iterator end() const
    {
        return Iterator(nodepntr(nullptr), _tree -> _left);
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
        _tree -> _left = _insert(_tree -> _left, key, value);
        _size++;
        return Iterator(_find(_tree -> _left, key), _tree);
    }
    
    void remove(const key_type& key)
    {
        _tree -> _left = _remove(_tree -> _left, key);
        _size--;
    }
    
    Iterator find(const key_type& key)
    {
        if (!_tree -> _left){
            return end();
        }
        return Iterator(_find(_tree -> _left, key), _tree);
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
    
    ~avl_tree(){
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
        unsigned int hl = height(_node -> _left);
        unsigned int hr = height(_node -> _right);
        _node -> _height = (hr > hl ? hr : hl) + 1;
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
