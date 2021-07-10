#include <cstdint>
#include <cstddef>
#include "smart_pointer.h"
#include <iostream>

using smart_pointer::smartPointer;

template<typename Key, typename T>
class avl_tree
{
    typedef struct node {
        bool deleted;

        Key key;
        T value;
        int height;
        using nodeptr = smartPointer<node>;
        nodeptr left;
        nodeptr right;

        node(Key k, T val){key = k; value = val; left = right = nullptr; height = 1; deleted = false;}
    } node;

    using nodeptr = smartPointer<node>;
    nodeptr _tree;
    size_t _size;

    // iterator class
    typedef class tag_avl_tree_iterator
    {
        nodeptr _pNode;
        nodeptr _root;

    public:
        // ctor
        explicit tag_avl_tree_iterator(nodeptr instance = nullptr, nodeptr tree = nullptr)
                : _pNode(instance), _root(tree)
        { }

        tag_avl_tree_iterator& operator=(const tag_avl_tree_iterator& other) {
            _pNode = other._pNode;
            return *this;
        }

        bool operator==(const tag_avl_tree_iterator& rhs) const {
            return _pNode == rhs._pNode;
        }

        bool operator!=(const tag_avl_tree_iterator& rhs) const {
            return _pNode != rhs._pNode;
        }

        T& operator*() const {
            return val();
        }

        T& val() const {
            return _pNode->value;
        }

        Key& key() const {
            return _pNode->key;
        }

        tag_avl_tree_iterator& operator++() {
            if(!_pNode) return *this;
            if (!_pNode->deleted && _pNode->right) {
                _pNode = _pNode->right;
                while (_pNode->left)
                    _pNode = _pNode->left;
                return *this;
            } else {
                nodeptr q = _root->left; // get start node
                nodeptr suc;

                while (q) {
                    if (q->key > _pNode->key) {
                        suc = q;
                        q = q->left;
                    } else if (q->key < _pNode->key)
                        q = q->right;
                    else
                        break;
                }
                _pNode = suc;
                return *this;
            }
        }

        tag_avl_tree_iterator operator++(int) {
            tag_avl_tree_iterator _copy = *this;
            ++(*this);
            return _copy;
        }

        tag_avl_tree_iterator operator--() {
            if(!_pNode) return *this;
            if (!_pNode->deleted && _pNode->left) {
                _pNode = _pNode->left;
                while (_pNode->right)
                    _pNode = _pNode->right;
                return *this;
            }
            else {
                nodeptr q = _root->left; // get start node
                nodeptr suc;

                while (q) {
                    if (q->key < _pNode->key) {
                        suc = q;
                        q = q->right;
                    }
                    else if (q->key > _pNode->key)
                        q = q->left;
                    else
                        break;
                }
                _pNode = suc;
                return *this;

            }
        }

        tag_avl_tree_iterator operator--(int) {
            tag_avl_tree_iterator _copy = *this;
            --(*this);
            return _copy;
        }
    } avl_tree_iterator;

    friend tag_avl_tree_iterator;
public:

    typedef T                   value_type;
    typedef Key                 key_type;
    typedef avl_tree_iterator   iterator;
    typedef size_t              size_type;

    avl_tree(): _tree(new node(Key(), T())), _size(0) {

    }
    avl_tree(avl_tree& tree): avl_tree() {
        auto it = tree.begin();
        auto end = tree.end();
        while (it != end) {
            insert(it.key(), it.val());
            it++;
        }
    }
    // iterators
    iterator begin() {
        return iterator(_findmin(_tree->left), _tree);
    }

    iterator end() {
        return iterator(nodeptr(nullptr), _tree->left);
    }

    size_type size() const {
        return _size;
    }

    bool empty() const {
        return _size == static_cast<size_type>(0);
    }

    void clear() {
        _size = 0U;
        _tree->left = NULL;
    }

    T& operator[](const key_type& k) {
        auto res = find(k);
        if(res == end()) res = insert(k, T());
        return res.val();
    }

    T& operator[](key_type&& k) {
        auto res = find(std::move(k));
        if(res == end()) res = insert(std::move(k), T());
        return res.val();
    }

    iterator insert(const key_type& key, const value_type& val) {
        _tree->left = _insert(_tree->left, key, val);
        _size++;
        return iterator(_find(_tree->left,key), _tree);
    }

    iterator find(const key_type& key) {
        if(!_tree->left) return end();
        return iterator(_find(_tree->left,key), _tree);
    }

    bool erase(const key_type& key) {
        _tree->left = _remove(_tree->left, key);
        _size--;
        return true;
    }

    bool erase(iterator position) {
        _tree->left = _remove(_tree->left, position.key());
        _size--;
        return true;
    }

    // Helper functions
private:
    int _height(nodeptr n) {
        return n ? n->height : 0;
    }

    int _balancefactor(nodeptr n) {
        return _height(n->right) - _height(n->left);
    }

    void _fixheight(nodeptr n) {
        n->height = (_height(n->left) > _height(n->right) ?
                     _height(n->left) : _height(n->right))+1;
    }

    nodeptr _RRotation(nodeptr n) {
        nodeptr tmp = n->left;
        n->left = tmp->right;
        tmp->right = n;
        _fixheight(n);_fixheight(tmp);
        return tmp;
    }

    nodeptr _LRotation(nodeptr n) {
        nodeptr tmp = n->right;
        n->right = tmp->left;
        tmp->left = n;
        _fixheight(n);_fixheight(tmp);
        return tmp;
    }

    nodeptr _insert(nodeptr n, Key k, T val) {
        if(!n) n = nodeptr(new node(k, val));
        if(k < n->key)
            n->left = _insert(n->left, k, val);
        else if(k > n->key)
            n->right = _insert(n->right, k, val);
        else {
            n->value = val;
            return n;
        }

        return _balance(n);
    }

    nodeptr _balance(nodeptr n) {
        _fixheight(n);
        if(_balancefactor(n) == 2)
        {
            if(_balancefactor(n->right) < 0)
                n->right = _RRotation(n->right);
            return _LRotation(n);
        }
        if (_balancefactor(n) == -2)
        {
            if(_balancefactor(n->left) > 0)
                n->left = _LRotation(n->left);
            return _RRotation(n);
        }
        return n;
    }

    nodeptr _find(nodeptr n, const key_type& key) {
        if(n->left && n->key > key)
            return _find(n->left, key);
        else if(n->right && n->key < key)
            return _find(n->right, key);
        else {
            if(n->key != key) return nodeptr(nullptr);
            return n;
        }
    }

    nodeptr _findmin(nodeptr n) {
        if(n)
            return n->left ? _findmin(n->left) : n;
        return nodeptr(nullptr);
    }

    nodeptr _findmax(nodeptr n) {
        if(n)
            return n->right ? _findmax(n->right) : n;
        return nodeptr(nullptr);
    }

    nodeptr _removemin(nodeptr n) {
        if(n->left == 0)
            return n->right;
        n->left = _removemin(n->left);
        return _balance(n);
    }

    nodeptr _remove(nodeptr n, Key k) {
        if(!n) return nodeptr(nullptr);
        if(k < n->key)
            n->left = _remove(n->left, k);
        else if(k > n->key)
            n->right = _remove(n->right,k);
        else
        {
            nodeptr tmpl = n->left;
            nodeptr tmpr = n->right;

            n->deleted = true;

            if(!tmpr) return tmpl;
            nodeptr min = _findmin(tmpr);
            min->right = _removemin(tmpr);
            min->left = tmpl;
            return _balance(min);
        }
        return _balance(n);
    }
};
