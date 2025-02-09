#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <future>
#include <functional>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    // Constructor. By default, the pool will have 'threadCount' threads.
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency())
        : stopFlag(false)
    {
        if(threadCount == 0) {
            // fallback if hardware_concurrency is 0 or user passed 0
            threadCount = 1;
        }

        workers.reserve(threadCount);
        for(size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] {
                // Each thread continuously tries to pop tasks from the queue and execute them
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] {
                            return this->stopFlag || !this->tasks.empty();
                        });
                        if(this->stopFlag && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    // Execute the retrieved task
                    task();
                }
            });
        }
    }

    // Destructor. Joins all threads.
    ~ThreadPool()
    {
        {
            // Signal all threads to stop
            std::unique_lock<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();

        // Join all threads
        for(std::thread &worker: workers) {
            if(worker.joinable()) {
                worker.join();
            }
        }
    }

    // Enqueue a task to be executed by the thread pool.
    // Returns a std::future that can be used to retrieve the result.
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using returnType = typename std::invoke_result<F, Args...>::type;

        // Create a packaged task
        auto taskPtr = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<returnType> res = taskPtr->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            // If the pool is stopping, don't accept new tasks
            if(stopFlag) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            // Add the task to the queue
            tasks.emplace([taskPtr](){ (*taskPtr)(); });
        }
        
        // Notify one worker thread that a new task is available
        condition.notify_one();

        return res;
    }

    // Returns the number of worker threads in the pool
    size_t size() const {
        return workers.size();
    }

private:
    // Workers (the actual threads)
    std::vector<std::thread> workers;
    
    // Task queue
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex queueMutex;
    std::condition_variable condition;

    // Stop flag
    bool stopFlag;
};

#endif // THREADPOOL_H
