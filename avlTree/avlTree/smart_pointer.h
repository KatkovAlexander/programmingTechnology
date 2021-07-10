#include <memory>
#include <utility>
#include <shared_mutex>

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
    explicit SmartPointer(value_type* pointer = nullptr) {
        if (pointer) {
            core = new Core;
            core->pointer = pointer;
            core->owners = 1;
        } else {
            core = nullptr;
        }
    }
    
    /*
     конструктор копирования
    */
    SmartPointer(const SmartPointer& sp) {
        shared_lock<shared_timed_mutex> lock (mut);{
            shared_lock<shared_timed_mutex> lock (sp.mut);
            {}
        }
        core = sp.core;
        if (core) core->owners++;
    }
    
    /*
     конструктор перемещения
    */
    SmartPointer(SmartPointer&& sp){
        unique_lock<shared_timed_mutex> lock (mut);
        core = sp.core;
        sp.core = nullptr;
    }
    
    
    SmartPointer& operator=(const SmartPointer& sp) {
        shared_lock<shared_timed_mutex> lock (mut);{
            shared_lock<shared_timed_mutex> lock (sp.mut);
            {}
        }
        if (!core) {
            core = sp.core;
            if (sp.core) core->owners++;
        } else {
            core->owners--;
            if (!core->owners) {
                delete core;
                core = nullptr;
            }
            
            core = sp.core;
            if (sp.core) core->owners++;
        }
        return *this;
    }
    
    SmartPointer& operator=(SmartPointer&& sp) {
        unique_lock<shared_timed_mutex> lock (mut);
        if (core) {
            core->owners--;
            if (!core->owners) {
                delete core;
                core = nullptr;
            }
        }
        
        core = sp.core;
        sp.core = nullptr;
        return *this;
    }
    
    SmartPointer& operator=(value_type* pointer) {
        unique_lock<shared_timed_mutex> lock (mut);
        if (core) {
            core->owners--;
            if (!core->owners) {
                delete core;
                core = nullptr;
            }
        }
        
        if (pointer) {
            core = new Core;
            core->pointer = pointer;
            core->owners = 1;
        }
        return *this;
    }
    
    ~SmartPointer() {
        unique_lock<shared_timed_mutex> lock (mut);
        if (core != nullptr) {
            core->owners--;
            if (core->owners == 0) {
                delete core;
                core = nullptr;
            }
        }
    }
    
    value_type& operator*() {
        shared_lock<shared_timed_mutex> lock (mut);
        if (!core) throw smart_pointer::exception();
        return *(core->pointer);
    }
    
    const value_type& operator*() const {
        shared_lock<shared_timed_mutex> lock (mut);
        if (!core) throw smart_pointer::exception();
        return *(core->pointer);
    }
    
    value_type* operator->() const {
        shared_lock<shared_timed_mutex> lock (mut);
        return core ? core->pointer : nullptr;
    }
    
    value_type* get() const {
        shared_lock<shared_timed_mutex> lock (mut);
        return core ? core->pointer : nullptr;
    }
    
    operator bool() const {
        shared_lock<shared_timed_mutex> lock (mut);
        return core != nullptr;
    }
    
    template <typename U>
    bool operator==(const SmartPointer<U>& sp) const {
        return (!core && sp.get() == nullptr) ||
        (static_cast<void*>(get()) == static_cast<void*>(sp.get()));
    }
    
    template <typename U>
    bool operator!=(const SmartPointer<U>& sp) const {
        return !((!core && sp.get() == nullptr) ||
                 (static_cast<void*>(get()) == static_cast<void*>(sp.get())));
    }
    
    std::size_t count_owners() const { return core ? core->owners : 0; }
    
private:
    class Core {
    public:
        size_t owners = 0;
        value_type* pointer;
    };
    Core* core;
    mutable shared_timed_mutex mut;
};
}
