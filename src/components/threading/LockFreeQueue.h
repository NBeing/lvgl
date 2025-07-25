#pragma once

#include <atomic>
#include <array>
#include <vector>

/**
 * @brief Lock-free single-producer single-consumer queue
 * 
 * High-performance queue for RT thread communication.
 * Thread-safe for one producer and one consumer.
 */
template<typename T, size_t Capacity>
class LockFreeQueue {
private:
    std::array<T, Capacity> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    
public:
    LockFreeQueue() = default;
    
    // No copy/move to keep it simple
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    /**
     * @brief Enqueue an item (producer side)
     * @param item Item to enqueue
     * @return true if successful, false if queue is full
     */
    bool enqueue(const T& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) % Capacity;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Dequeue an item (consumer side)
     * @param item Reference to store the dequeued item
     * @return true if successful, false if queue is empty
     */
    bool dequeue(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) % Capacity, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get approximate size (may be slightly inaccurate due to concurrency)
     */
    size_t size() const {
        const size_t current_tail = tail_.load(std::memory_order_acquire);
        const size_t current_head = head_.load(std::memory_order_acquire);
        
        if (current_tail >= current_head) {
            return current_tail - current_head;
        } else {
            return Capacity - current_head + current_tail;
        }
    }
    
    /**
     * @brief Get maximum capacity
     */
    constexpr size_t capacity() const {
        return Capacity - 1; // One slot is reserved to distinguish full/empty
    }
};

/**
 * @brief Simplified template specialization for dynamic capacity
 */
template<typename T>
class LockFreeQueue<T, 0> {
private:
    std::vector<T> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    size_t capacity_;
    
public:
    explicit LockFreeQueue(size_t capacity) 
        : buffer_(capacity + 1), capacity_(capacity + 1) {} // +1 for the reserved slot
    
    bool enqueue(const T& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) % capacity_;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    bool dequeue(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        item = buffer_[current_head];
        head_.store((current_head + 1) % capacity_, std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        const size_t current_tail = tail_.load(std::memory_order_acquire);
        const size_t current_head = head_.load(std::memory_order_acquire);
        
        if (current_tail >= current_head) {
            return current_tail - current_head;
        } else {
            return capacity_ - current_head + current_tail;
        }
    }
    
    size_t capacity() const {
        return capacity_ - 1;
    }
};
