#include <thread>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>

template <typename T>
class ThreadsafeQueue
{
  private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

  public:
    ThreadsafeQueue();
    ThreadsafeQueue(const ThreadsafeQueue &);
    ThreadsafeQueue & operator = (const ThreadsafeQueue &) = delete;
    void push(T new_value);
    bool try_pop(T &value);
    std::shared_ptr<T> try_pop();
    void wait_and_pop(T &value);
    std::shared_ptr<T> wait_and_pop();
    bool empty() const;
};

template <typename T>
ThreadsafeQueue<T>::ThreadsafeQueue() {}

template <typename T>
ThreadsafeQueue<T>::ThreadsafeQueue(const ThreadsafeQueue &other)
{
  std::lock_guard<std::mutex> lk(other.mut);
  this->data_queue = other.data_queue;
}

template <typename T>
void ThreadsafeQueue<T>::push(T new_value)
{
  std::lock_guard<std::mutex> lk(mut);
  data_queue.push(new_value);
  data_cond.notify_one(); // notify the waiting threads as data is here
}

template <typename T>
void ThreadsafeQueue<T>::wait_and_pop(T &value)
{
  std::unique_lock<std::mutex> lk(mut);
  this->data_cond.wait(lk, [this]{return ~this->data_queue.empty();});
  value = this->data_queue.front();
  this->data_queue.pop();
}

template <typename T>
std::shared_ptr<T> ThreadsafeQueue<T>::wait_and_pop()
{
  std::unique_lock<std::mutex> lk(mut);
  this->data_cond.wait(lk, [this]{return !this->data_queue.empty();});
  std::shared_ptr<T> res(std::make_shared<T>(this->data_queue.front()));
  this->data_queue.pop();
  return res;
}

template <typename T>
bool ThreadsafeQueue<T>::try_pop(T &value)
{
  std::lock_guard<std::mutex> lk(mut);
  if (data_queue.empty())
    return false;
  value = data_queue.front();
  data_queue.pop();
  return true;
}

template <typename T>
std::shared_ptr<T> ThreadsafeQueue<T>::try_pop()
{
  std::lock_guard<std::mutex> lk(mut);
  if (data_queue.empty())
    return std::shared_ptr<T>();
  std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
  data_queue.pop();
  return res;
}

template <typename T>
bool ThreadsafeQueue<T>::empty() const
{
  std::lock_guard<std::mutex> lk(mut);
  return data_queue.empty();
}

int main() 
{
  std::mutex cout_mutex;  // Add this at the start of main
  ThreadsafeQueue<int> queue;
  auto producer = [&queue, &cout_mutex](int start, int count) {
      for(int i = 0; i < count; i++) {
          queue.push(start + i);
          {
              std::lock_guard<std::mutex> lock(cout_mutex);
              std::cout << "Produced: " << (start + i) << std::endl;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
  };

  auto consumer = [&queue, &cout_mutex](int id) {
      for(int i = 0; i < 5; i++) {
          int value;
          queue.wait_and_pop(value);
          {
              std::lock_guard<std::mutex> lock(cout_mutex);
              std::cout << "Consumer " << id << " got value: " << value << std::endl;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
  };

    std::thread producer1(producer, 1, 5);    // Produces: 1,2,3,4,5
    std::thread producer2(producer, 100, 5);  // Produces: 100,101,102,103,104
    std::thread consumer1(consumer, 1);
    std::thread consumer2(consumer, 2);

    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();

    int value;
    if(queue.try_pop(value)) {
        std::cout << "try_pop successful, got: " << value << std::endl;
    } else {
        std::cout << "try_pop failed, queue was empty" << std::endl;
    }

    if(!queue.empty()) {
        auto ptr = queue.wait_and_pop();
        std::cout << "Shared_ptr pop successful, got: " << *ptr << std::endl;
    }

    return 0;
}
