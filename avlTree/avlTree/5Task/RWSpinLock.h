#pragma once

#include <stdexcept>
#include <thread>
#include <shared_mutex>


struct RWSpinLock {
    
    void writeLock () {
        
        while (true) {
            uint32_t oldVal = val;
            uint32_t newVal = oldVal | WBIT;
            
            if (!(oldVal & WBIT) && val.compare_exchange_strong(oldVal, newVal)) {
                break;
            }
            
            std::this_thread::yield();
        }
        
        while (true) {
            if (val == WBIT) {
                break;
            }
            
            std::this_thread::yield();
        }
    }
    
    void reedLock () {
        while (true)
        {
            uint32_t oldVal = val;
            uint32_t newVal = oldVal + 1;
            
            if (!(oldVal & WBIT) && val.compare_exchange_strong(oldVal, newVal)) {
                break;
            }
            
            if (oldVal & WBIT) {
                std::this_thread::yield();
            }
        }
    }
    
    void unlock () {
        if (val == WBIT) {
            val = 0;
        } else {
            val--;
        }
    }
    
private:
    std::atomic<uint32_t> val;
    uint32_t WBIT = 1 << 31;
};
