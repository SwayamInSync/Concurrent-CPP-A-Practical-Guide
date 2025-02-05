#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>

// Mutex for clean console output
std::mutex cout_mutex;

// Helper for clean console output
void print(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << msg << std::endl;
}

// Simulates a computation that multiple threads need to use
int compute_shared_value() {
    print("Starting computation of shared value...");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    print("Computation complete!");
    return 42;
}

// Worker function that uses the shared future
void worker(int id, std::shared_future<int> fut) {  // Note: fut is passed by value!
    print("Thread " + std::to_string(id) + " starting and waiting for value...");
    
    // Each thread has its own copy of the shared_future, so this is thread-safe
    int value = fut.get();  // Will block until the value is available
    
    print("Thread " + std::to_string(id) + " got value: " + std::to_string(value));
    
    // Simulate some work with the received value
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    print("Thread " + std::to_string(id) + " finished processing");
}

// Example using promise and shared_future
void shared_future_example() {
    print("\n=== Promise with shared_future example ===");
    
    std::promise<int> promise;
    std::shared_future<int> shared_fut = promise.get_future().share();
    
    // Launch multiple threads that will wait for the same value
    std::vector<std::thread> threads;
    for(int i = 0; i < 3; ++i) {
        threads.emplace_back(worker, i, shared_fut);  // Each thread gets its own copy
    }
    
    // Simulate some preparation work
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Set the value that all threads are waiting for
    print("Main thread setting the value...");
    promise.set_value(42);
    
    // Wait for all threads to finish
    for(auto& t : threads) {
        t.join();
    }
}

// Example using async with shared_future
void shared_future_from_async() {
    print("\n=== Async with shared_future example ===");
    
    // Launch async task and get shared_future
    std::shared_future<int> shared_fut = 
        std::async(std::launch::async, compute_shared_value).share();
    
    // Launch threads that will use the result
    std::vector<std::thread> threads;
    for(int i = 0; i < 3; ++i) {
        threads.emplace_back(worker, i, shared_fut);
    }
    
    // Wait for all threads to finish
    for(auto& t : threads) {
        t.join();
    }
}

int main() {
    // Show both examples
    shared_future_example();
    shared_future_from_async();
    
    return 0;
}
