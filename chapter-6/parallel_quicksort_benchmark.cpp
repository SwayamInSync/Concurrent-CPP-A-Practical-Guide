#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <list>
#include <future>
#include <random>
#include <iomanip>

template <typename T>
std::list<T> quick_sort(std::list<T> input)
{
  if(input.empty())
  {
    return input;
  }

  std::list<T> result;
  result.splice(result.begin(), input, input.begin());
  T const& pivot = *result.begin();
  auto divide_point = std::partition(input.begin(), input.end(), 
    [&](T const& t){return t < pivot;});

  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
  auto new_lower = quick_sort(std::move(lower_part));
  auto new_higher = quick_sort(std::move(input));

  result.splice(result.begin(), new_lower);
  result.splice(result.end(), new_higher);
  return result;
}

template <typename T>
std::list<T> quick_sort_parallel(std::list<T> input)
{
    if(input.empty() || input.size() < 10000)
    {
        return quick_sort(input);
    }

    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    
    auto divide_point = std::partition(input.begin(), input.end(), 
        [&](T const& t){return t < pivot;});

    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    
    std::future<std::list<T>> new_lower_future;
    if(lower_part.size() > 10000) {
        new_lower_future = std::async(std::launch::async, 
            &quick_sort_parallel<T>, std::move(lower_part));
    } else {
        new_lower_future = std::async(std::launch::deferred,
            &quick_sort_parallel<T>, std::move(lower_part));
    }
    
    auto new_higher = quick_sort_parallel(std::move(input));

    result.splice(result.begin(), new_lower_future.get());
    result.splice(result.end(), new_higher);
    return result;
}


int main() {
    unsigned int num_threads = std::thread::hardware_concurrency();
    std::cout << "Hardware Concurrency: " << num_threads << " threads\n";

    const size_t SIZE = 10'000'000;
    std::list<int> data1, data2;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, SIZE);

    for(size_t i = 0; i < SIZE; ++i) {
        int num = dist(gen);
        data1.push_back(num);
        data2.push_back(num); 
    }

    // Sequential Sort
    std::cout << "\nStarting Sequential Sort..." << std::endl;
    auto start_seq = std::chrono::high_resolution_clock::now();
    auto result_seq = quick_sort(std::move(data1));
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto time_seq = std::chrono::duration_cast<std::chrono::milliseconds>(end_seq - start_seq);

    // Parallel Sort
    std::cout << "Starting Parallel Sort..." << std::endl;
    auto start_par = std::chrono::high_resolution_clock::now();
    auto result_par = quick_sort_parallel(std::move(data2));
    auto end_par = std::chrono::high_resolution_clock::now();
    auto time_par = std::chrono::duration_cast<std::chrono::milliseconds>(end_par - start_par);

    std::cout << "\nResults for sorting " << SIZE << " integers:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Sequential Time: " << time_seq.count() << " ms\n";
    std::cout << "Parallel Time:   " << time_par.count() << " ms\n";
    std::cout << "Speedup:         " << std::fixed << std::setprecision(2) 
              << static_cast<double>(time_seq.count()) / time_par.count() << "x\n";

    bool are_equal = (result_seq == result_par);
    bool is_seq_sorted = std::is_sorted(result_seq.begin(), result_seq.end());
    bool is_par_sorted = std::is_sorted(result_par.begin(), result_par.end());

    std::cout << "\nValidation:\n";
    std::cout << "Results match:     " << (are_equal ? "Yes" : "No") << "\n";
    std::cout << "Sequential sorted: " << (is_seq_sorted ? "Yes" : "No") << "\n";
    std::cout << "Parallel sorted:   " << (is_par_sorted ? "Yes" : "No") << "\n";

    return 0;
}
