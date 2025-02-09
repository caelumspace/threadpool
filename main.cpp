#include <iostream>
#include "ThreadPool.h"

int main() {
    // Create a thread pool with 4 threads
    ThreadPool pool(4);

    // Enqueue a few simple tasks
    auto f1 = pool.enqueue([] {
        std::cout << "Task 1 is running\n";
        return 1;
    });

    auto f2 = pool.enqueue([] {
        std::cout << "Task 2 is running\n";
        return 2;
    });

    auto f3 = pool.enqueue([](int x) {
        std::cout << "Task 3 is running with parameter: " << x << "\n";
        return x * 2;
    }, 10);

    // Retrieve the results
    std::cout << "Result of f1: " << f1.get() << "\n";
    std::cout << "Result of f2: " << f2.get() << "\n";
    std::cout << "Result of f3: " << f3.get() << "\n";

    return 0;
}
