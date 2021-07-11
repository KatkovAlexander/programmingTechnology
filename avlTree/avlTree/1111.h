#pragma once

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <cassert>

#include "RwSpinLock.h";

using lock_write = std::unique_lock<std::shared_mutex>;
using lock_read = std::shared_lock<std::shared_mutex>;

template <typename T>
class FineGrainingListWithSpinLock
{
    template <typename T>
    class Node
    {
    private:
        template<typename ValueType>
        friend class FineGrainingListWithSpinLock;

    public:
        Node(FineGrainingListWithSpinLock* list_ref) :
            prev(nullptr),
            next(nullptr),
            list_ref(list_ref) { }

        Node(T data, FineGrainingListWithSpinLock* list_ref) :
            data(data),
            prev(nullptr),
            next(nullptr),
            list_ref(list_ref) { }

        static void acquire(Node** destination, Node* new_node)
        {
            *destination = new_node;

            new_node->ref_count++;
        }

        void release(Node* node, bool isNeedLock = true)
        {
            if (isNeedLock)
                list_ref->global_mutex.rlock();

            std::uint32_t old_ref_count = --node->ref_count;

            if ((old_ref_count & (~(1 << 31))) == 0)
            {
                list_ref->purgatory->killNode(node);
            }

            if (isNeedLock)
                list_ref->global_mutex.unlock();
        }

        bool isDeleted()
        {
            return deleted();
        }

        void increaseRefCount()
        {
            ref_count++;
        }

        void decreaseRefCount()
        {
            ref_count--;
        }

        int32_t refCount()
        {
            return ref_count & (~(1 << 31));
        }

        int32_t deleted()
        {
            return ref_count & (1 << 31);
        }

        void setDeleted()
        {
            ref_count |= (1 << 31);
        }

        T data;
        Node* prev;
        Node* next;
        FineGrainingListWithSpinLock* list_ref;
        std::atomic<std::uint32_t> ref_count = 0;
        std::atomic<std::uint32_t> purged = 0;
        std::shared_mutex node_mutex;
    };

    template <typename T>
    class Purgatory
    {
    private:
        template<typename ValueType>
        friend class FineGrainingListWithSpinLock;

    public:
        using Nod = Node<T>;

        template <typename T>
        class PurgatoryNode
        {
        public:
            PurgatoryNode(Nod* body) :
                body(body) { }

            Nod* body = nullptr;
            PurgatoryNode* next = nullptr;
        };

        using PNode = PurgatoryNode<T>;

        Purgatory(FineGrainingListWithSpinLock* list_ref) :
            list_ref(list_ref),
            head(nullptr),
            cleanThread(&Purgatory::PurgatoryCleanThread, this) { }

        ~Purgatory()
        {
            setTerminated();

            cleanThread.join();
        }

        void setTerminated()
        {
            terminated = true;
        }

        void killNode(Nod* body)
        {
            PNode* node = new PNode(body);

            do {
                node->next = head.load();
            } while (!head.compare_exchange_strong(node->next, node));
        }

        void remove(PNode* prev, PNode* node)
        {
            prev->next = node->next;

            free(node);
        }

        void freeNode(PNode* node)
        {
            Node<T>* prev = node->body->prev;
            Node<T>* next = node->body->next;

            if (prev != nullptr)
            {
                prev->release(prev);
            }

            if (next != nullptr)
            {
                next->release(next);
            }

            free(node->body);
            free(node);
        }

        void PurgatoryCleanThread()
        {
            do
            {
                list_ref->global_mutex.wlock();

                PNode* purge_start = this->head;

                list_ref->global_mutex.unlock();

                if (purge_start != nullptr)
                {
                    int k = 0;

                    PNode* prev = purge_start;
                    for (PNode* node = purge_start; node != nullptr;)
                    {
                        k++;
                        PNode* ovdVal = node;
                        node = node->next;

                        if (ovdVal->body->refCount() > 0 || ovdVal->body->purged == 1)
                        {
                            remove(prev, ovdVal);
                        }
                        else
                        {
                            ovdVal->body->purged = 1;
                            prev = ovdVal;
                        }
                    }
                    prev->next = nullptr;


                    // second faze

                    list_ref->global_mutex.wlock();

                    PNode* new_purge_start = this->head;

                    if (new_purge_start == purge_start)
                    {
                        // no new nodes after first faze
                        // => head become free

                        this->head = nullptr;
                    }

                    list_ref->global_mutex.unlock();

                    prev = new_purge_start;
                    PNode* node = new_purge_start;
                    while (node != purge_start)
                    {
                        PNode* ovdVal = node;
                        node = node->next;

                        if (ovdVal->body->purged == 1)
                        {
                            remove(prev, ovdVal);
                        }
                        else
                        {
                            prev = ovdVal;
                        }
                    }
                    prev->next = nullptr;


                    for (PNode* node = purge_start; node != nullptr;)
                    {
                        PNode* ovdVal = node;
                        node = node->next;

                        freeNode(ovdVal);
                    }
                }

                if (!terminated)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } while (!terminated || head.load() != nullptr);
        }

        std::thread cleanThread;
        std::atomic<PurgatoryNode<T>*> head;
        std::atomic<int32_t> global_counter = 0;
        FineGrainingListWithSpinLock* list_ref;
        bool terminated = false;
    };

public:
    template <typename T>
    class Iterator
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Node<T>;
        using difference_type = std::ptrdiff_t;
        using reference = Node<T>&;
        using pointer = Node<T>*;

        Iterator(const Iterator& other) :
            ptr(other.ptr), list_ref(other.list_ref)
        {
            ptr->increaseRefCount();
        }

        Iterator(Iterator&& other) :
            ptr(other.ptr), list_ref(other.list_ref)
        {
            ptr->increaseRefCount();
        }

        Iterator& operator =(const Iterator& other)
        {
            if (ptr == other.ptr)
                return *this;

            Nod* old_ptr = ptr;

            ptr = other.ptr;
            list_ref = other.list_ref;

            if (old_ptr != nullptr)
                ptr->release(ptr);

            return *this;
        }

        Iterator& operator =(Iterator&& other)
        {
            if (ptr == other.ptr)
                return *this;

            Nod* old_ptr = ptr;

            ptr = other.ptr;
            list_ref = other.list_ref;

            if (old_ptr != nullptr)
                ptr->release(ptr);

            return *this;
        }

        ~Iterator()
        {
            ptr->release(ptr);
        }

        T operator*() const
        {
            return ptr->data;
        }

        pointer operator->() const
        {
            return ptr;
        }

        Iterator& operator++()
        {
            pointer node = ptr;

            list_ref->global_mutex.rlock();

            pointer next_node = node->next;

            value_type::acquire(&ptr, next_node);

            list_ref->global_mutex.unlock();

            node->release(node);

            return *this;
        }

        Iterator operator++(int num)
        {
            ++(*this);
            return *this;
        }

        Iterator& operator--()
        {
            pointer node = ptr;
            list_ref->global_mutex.rlock();

            pointer prev_node = node->prev;

            value_type::acquire(&ptr, prev_node);

            list_ref->global_mutex.unlock();

            node->release(node);

            return *this;
        }

        Iterator operator--(int num)
        {
            --(*this);
            return *this;
        }

        bool operator ==(const Iterator& other)
        {
            return ptr == other.ptr;
        }

        bool operator !=(const Iterator& other)
        {
            return !(*this == other);
        }

    private:
        template<typename ValueType>
        friend class FineGrainingListWithSpinLock;


        Iterator(pointer ptr, FineGrainingListWithSpinLock* list_ref) :
            ptr(ptr), list_ref(list_ref)
        {
            ptr->increaseRefCount();
        }

        pointer ptr;
        FineGrainingListWithSpinLock* list_ref;
    };

    using Nod = Node<T>;
    using Iter = Iterator<T>;

    FineGrainingListWithSpinLock() :
        start(new Nod(this)),
        finish(new Nod(this)),
        purgatory(new Purgatory<T>(this))
    {
        start->ref_count = 1;
        start->prev = nullptr;
        start->next = nullptr;

        finish->ref_count = 1;
        finish->prev = nullptr;
        finish->next = nullptr;

        list_size = 0;

        Nod::acquire(&start->next, finish);
        Nod::acquire(&finish->prev, start);
    }

    FineGrainingListWithSpinLock(T data) :
        start(new Nod(this)),
        finish(new Nod(this)),
        purgatory(new Purgatory<T>(this))
    {
        start->ref_count = 1;
        start->prev = nullptr;
        start->next = nullptr;

        finish->ref_count = 1;
        finish->prev = nullptr;
        finish->next = nullptr;

        list_size = 0;

        Nod::acquire(&start->next, finish);
        Nod::acquire(&finish->prev, start);

        push_back(data);
    }

    ~FineGrainingListWithSpinLock()
    {
        delete(purgatory);

        auto node = start->next;
        while (node != finish)
        {
            auto buf = node;

            node = node->next;

            delete buf;
        }

        delete start;
        delete finish;
    }

    Iter begin()
    {
        lock_read lock(start->node_mutex);

        if (start->next == finish)
        {
            throw std::out_of_range("out of range");
        }

        Nod* ret = start->next;
        Iter it = Iter(ret, this);

        lock.unlock();

        return it;
    }

    Iter rbegin()
    {
        return Iter(start, this);
    }

    Iter end()
    {
        return Iter(finish, this);
    }

    Iter rend()
    {
        lock_read lock(finish->node_mutex);

        if (finish->prev == start)
        {
            throw std::out_of_range("out of range");
        }

        Nod* ret = finish->prev;
        Iter it = Iter(ret, this);

        lock.unlock();

        return it;
    }

    bool empty()
    {
        return !list_size;
    }

    int size()
    {
        return list_size;
    }

    void push_front(T data)
    {
        auto it = Iter(start, this);
        insert(it, data);
    }

    void push_back(T data)
    {
        auto it = Iter(finish->prev, this);
        insert(it, data);
    }

    Iter erase(Iter& iter)
    {
        Nod* node = iter.ptr;

        for (bool retry = true; retry; )
        {
            lock_read lock_node(node->node_mutex);

            if (node == finish || node == start)
            {
                throw std::out_of_range("out of range");
            }

            Nod* prev = node->prev;
            assert(prev->refCount());
            prev->ref_count++;

            Nod* next = node->next;
            assert(next->ref_count);
            next->ref_count++;

            lock_node.unlock();

            lock_write lock_prev(prev->node_mutex);
            lock_node.lock();
            lock_write lock_next(next->node_mutex);

            if (node->isDeleted())
            {
                // iter++
                Nod::acquire(&iter.ptr, next);

                lock_next.unlock();
                lock_node.unlock();
                lock_prev.unlock();

                prev->release(prev);
                next->release(next);

                node->release(node);
                return iter;
            }

            if (prev->next == node && next->prev == node)
            {
                Nod::acquire(&prev->next, next);
                Nod::acquire(&next->prev, prev);

                node->setDeleted();

                node->release(node);
                node->release(node);

                list_size--;

                retry = false;

                // iter++
                Nod::acquire(&iter.ptr, next);
            }

            lock_next.unlock();
            lock_node.unlock();
            lock_prev.unlock();

            prev->release(prev);
            next->release(next);
        }

        node->release(node);
        return iter;
    }

    Iter insert(Iter& iter, T data)
    {
        Nod* prev = iter.ptr;

        if (iter.ptr == finish)
        {
            throw std::out_of_range("out of range");
        }

        for (bool retry = true; retry; )
        {
            if (iter.ptr->isDeleted())
            {
                return Iter(finish, this);
            }

            lock_write lock_prev(prev->node_mutex);

            Nod* next = prev->next;
            lock_write lock_next(next->node_mutex);

            if (prev == next->prev)
            {
                Nod* new_node = new Nod(data, this);
                lock_write lock_new(new_node->node_mutex);

                Nod::acquire(&new_node->prev, prev);
                Nod::acquire(&new_node->next, next);

                Nod::acquire(&prev->next, new_node);
                Nod::acquire(&next->prev, new_node);

                prev->release(prev);
                next->release(next);

                list_size++;
                retry = false;

                // iter++
                Nod::acquire(&iter.ptr, new_node);

                lock_new.unlock();
            }

            lock_next.unlock();
            lock_prev.unlock();
        }

        prev->release(prev);
        return iter;
    }

private:
    Nod* start;
    Nod* finish;
    std::atomic<std::size_t> list_size = 0;
    rwpinLock global_mutex;
    Purgatory<T>* purgatory;
};
