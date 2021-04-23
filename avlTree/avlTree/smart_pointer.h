#pragma once

#include <memory>
#include <utility>

namespace smart_pointer {
    
class exception : std::exception {
    using base_class = std::exception;
    using base_class::base_class;
};
    
template<typename T>
class smartPointer{
public:
    using value_type = T;
    explicit smartPointer(value_type* pointer = nullptr) {
        if (pointer) {
            core = new Core(pointer);
            core->pointer = pointer;
            core->owners = 1;
        } else {
            core = nullptr;
        }
    }

    smartPointer(const smartPointer& src) : core(src.core) {
        if (core != nullptr && core->pointer != nullptr)
            core->owners++;
    }

    smartPointer(smartPointer&& sp) {
        core = std::move(sp.core);
        sp.core = nullptr;
    }

    smartPointer& operator=(const smartPointer& sp) {
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

    smartPointer& operator=(smartPointer&& sp) {
        if (core) {
            core->owners--;
            if (!core->owners) {
                delete core;
                core = nullptr;
            }
        }

        std::swap(core, sp.core);
        return *this;
    }

    smartPointer& operator=(value_type* pointer) {
        if (core) {
            core->owners--;
            if (!core->owners) {
                delete core;
                core = nullptr;
            }
        }

        if (pointer) {
            core = new Core(pointer);
            core->pointer = pointer;
            core->owners = 1;
        }
        return *this;
  }

//    ~smartPointer() {
//        if (core) {
//            core->owners--;
//            if (!core->owners) {
//                delete core;
//            }
//        }
//    }

    value_type& operator*() {
        if (!core) throw smart_pointer::exception();
        return *(core->pointer);
    }
  
    const value_type& operator*() const {
        if (!core) throw smart_pointer::exception();
        return *(core->pointer);
    }

    value_type* operator->() const { return core ? core->pointer : nullptr; }

    value_type* get() const { return core ? core->pointer : nullptr; }

    operator bool() const { return core != nullptr; }

    template <typename U>
    bool operator==(const smartPointer<U>& sp) const {
        return (!core && sp.get() == nullptr) ||
            (static_cast<void*>(get()) == static_cast<void*>(sp.get()));
    }

    template <typename U>
    bool operator!=(const smartPointer<U>& sp) const {
        return !((!core && sp.get() == nullptr) ||
                 (static_cast<void*>(get()) == static_cast<void*>(sp.get())));
  }

  std::size_t count_owners() const { return core ? core->owners : 0; }

 private:
  
    class Core {
    public:
        explicit Core(value_type* ptr) : pointer(ptr){
            if (this -> pointer != nullptr) {
                owners++;
            }
        };
        
        std::size_t owners = 0;
        value_type* pointer = nullptr;
  };
    
  Core* core;
};

}

