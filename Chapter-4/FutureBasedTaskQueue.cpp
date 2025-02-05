#include <iostream>
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <chrono>

class TaskQueue {
private:
    std::mutex m;
    std::deque<std::packaged_task<void()>> tasks;
    bool stop_flag = false;

public:
    // Function to add tasks to the queue
    std::future<void> post_task(std::function<void()> f) {
        std::packaged_task<void()> task(f);
        std::future<void> res = task.get_future();
        {
            std::lock_guard<std::mutex> lk(m);
            tasks.push_back(std::move(task));
        }
        return res;
    }

    // Worker thread function
    void worker_thread() {
        while (!stop_flag) {
            std::packaged_task<void()> task;
            {
                std::lock_guard<std::mutex> lk(m);
                if (tasks.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                task = std::move(tasks.front());
                tasks.pop_front();
            }
            task();  // Execute the task
        }
    }

    void stop() { stop_flag = true; }
};

int main() {
    TaskQueue tq;
    // Start worker thread
    std::thread worker(&TaskQueue::worker_thread, &tq);

    // Post some tasks
    auto f1 = tq.post_task([]{ 
        std::cout << "Task 1 executing\n"; 
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    
    auto f2 = tq.post_task([]{ 
        std::cout << "Task 2 executing\n"; 
    });

    // Wait for tasks to complete
    f1.get();
    f2.get();

    // Cleanup
    tq.stop();
    worker.join();
    return 0;
}
