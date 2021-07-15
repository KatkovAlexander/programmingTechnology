#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>

using std::unique_lock;
using std::shared_lock;
using std::shared_timed_mutex;

namespace smart_pointer {
class exception : std::exception {
    using base_class = std::exception;
    using base_class::base_class;
};


template<typename T>
class SmartPointer {
    
public:
    using value_type = T;
    
    /*
     Конструктор работает с указателем
    */

    explicit SmartPointer(value_type* val = nullptr) :
    core(val == nullptr ? nullptr : new Core(val, 1)) {}
    
    /*
     конструктор копирования
    */
    SmartPointer(const SmartPointer& sp) {
        std::shared_lock<shared_timed_mutex> lock (mut);
        std::shared_lock<shared_timed_mutex> lock2 (sp.mut);
        
        core = sp.core;
        if (core) core -> owners++;
    }
    
    /*
     конструктор перемещения
    */
    SmartPointer(SmartPointer&& sp) {
        std::unique_lock<shared_timed_mutex> lock(mut);
        core = sp.core;
        sp.core = nullptr;
    }
    
    // copy assigment
    SmartPointer &operator=(const SmartPointer &sp) {
        std::shared_lock<shared_timed_mutex> lock1 (mut);
        std::shared_lock<shared_timed_mutex> lock2 (sp.mut);
        
        if (core != nullptr) {
            core -> owners--;
            if (core -> owners == 0) {
                delete core -> pointer;
                delete core;
            }
        }
        core = sp.core;
        if (core)
            core -> owners++;
        
        return *this;
    }
    
    // move assigment
    SmartPointer &operator=(SmartPointer&& sp) {
        std::unique_lock<shared_timed_mutex> lock(mut);
        if (core != nullptr) {
            core -> owners--;
            if (core -> owners == 0) {
                delete core -> pointer;
                delete core;
            }
        }
        core = sp.core;
        sp.core = nullptr;
        return *this;
    }
    
    SmartPointer &operator=(value_type* sp) {
        std::unique_lock<shared_timed_mutex> lock(mut);
        if (core != nullptr) {
            core -> owners--;
            if (core -> owners == 0) {
                delete core -> pointer;
                delete core;
            }
        }
        if (sp == nullptr) {
            core = nullptr;
            return *this;
        }
        
        core = new Core(sp, 1);
        return *this;
    }
    
    ~SmartPointer() {
        std::unique_lock<shared_timed_mutex> lock(mut);
        if (core != nullptr) {
            core -> owners--;
            if (core -> owners == 0) {
                delete core -> pointer;
                delete core;
            }
        }
    }

    value_type& operator*() {
        std::shared_lock<shared_timed_mutex> lock (mut);
        if (!core) throw smart_pointer::exception();
        return *(core->ptr);
    }
    
    const value_type &operator*() const {
        std::shared_lock<shared_timed_mutex> lock (mut);
        if (!core) throw smart_pointer::exception();
        return *(core->ptr);
    }
    
    value_type *operator->() {
        std::shared_lock<shared_timed_mutex> lock(mut);
        return core ? core->pointer : nullptr;
    }
    
    
    value_type *operator->() const {
        std::shared_lock<shared_timed_mutex> lock(mut);
        return core ? core->pointer : nullptr;
    }
    
    
    value_type *get() const {
        std::shared_lock<shared_timed_mutex> lock(mut);
        return core ? core->pointer : nullptr;
    }
    
    // if pointer == nullptr => return false
    operator bool() const {
        std::shared_lock<shared_timed_mutex> lock(mut);
        return core != nullptr;
    }
    
    // if pointers points to the same address or both null => true
    template<typename U>
    bool operator==(const SmartPointer<U> &sp) const {
        return (!core && sp.get() == nullptr) ||
        (static_cast<void*>(get()) == static_cast<void *>(sp.get()));
    }
    
    // if pointers points to the same address or both null => false
    template<typename U>
    bool operator!=(const SmartPointer<U> &sp) const {
        return !((!core && sp.get() == nullptr) ||
                 (static_cast<void*>(get()) == static_cast<void*>(sp.get())));
    }
    
    std::size_t count_owners() const { return core ? core->owners : 0; }
    
private:
    class Core {
    public:
        Core(value_type *sp, size_t count) : pointer(sp), owners(count) {}
        std::atomic<size_t> owners = {0};
        value_type *pointer;

    };
    Core *core;
    mutable std::shared_timed_mutex mut;
};
}
