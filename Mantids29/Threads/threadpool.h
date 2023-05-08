#pragma once

#include <atomic>
#include <string>
#include <random>
#include <thread>
#include <queue>
#include <map>
#include <condition_variable>

namespace Mantids29 { namespace Threads {

namespace Pool {


// TODO: statistics
/**
 * @brief Advanced Thread Pool.
 */
class ThreadPool
{
public:

    struct Task
    {
        Task()
        {
            task = nullptr;
            data = nullptr;
        }

        bool isNull()
        {
            return task == nullptr && data == nullptr;
        }

        void (*task) (void *);
        void * data;
    };

    struct TasksQueue
    {
        TasksQueue()
        {
            init=true;
        }
        std::queue<Task> tasks;
        std::condition_variable cond_removedElement;
        bool init;
    };

    /**
     * @brief ThreadPool Initialize thread pool
     * @param threadsCount concurrent threads initialized
     * @param taskQueues available queues
     */
    ThreadPool(uint32_t threadsCount = 52, uint32_t taskQueues = 36);
    ~ThreadPool();
    /**
     * @brief start Start task consumer threads
     */
    void start();
    /**
     * @brief stop Terminate to process current tasks and stop task consumer threads (also called from destructor)
     */
    void stop();
    /**
     * @brief addTask Add Task
     * @param task task function
     * @param data data passed to task
     * @param timeoutMS timeout if insertion queue is full
     * @param key key used to determine the priority schema
     * @param priority value between (0-1] to determine how many queues are available for insertion
     * @return true if inserted, false if timed out or during stop
     */
    bool pushTask( void (*task)(void *), void * data , uint32_t timeoutMS = static_cast<uint32_t>(-1), const float & priority=0.5, const std::string & key = "");
    /**
     * @brief popTask function used by thread processor
     * @return task
     */
    Task popTask();
    /**
     * @brief getTasksByQueueLimit Get how many task will receive a queue
     * @return max task count
     */
    uint32_t getTasksByQueueLimit() const;
    /**
     * @brief getTasksByQueueLimit Set max tasks will receive a queue
     */
    void setTasksByQueueLimit(const uint32_t &value);

private:

    static void taskProcessor(ThreadPool * tp);

    size_t getRandomQueueByKey(const std::string & key, const float & priority);
    TasksQueue * getRandomTaskQueueWithElements(  );

    // TERMINATION:
    bool terminate;

    // LIMITS:
    std::atomic<uint32_t> tasksByQueueLimit;

    // THREADS:
    std::map<size_t,std::thread> threads;
    uint32_t threadsCount;

    // QUEUE OPERATIONS/CONDITIONS:
    std::map<size_t,TasksQueue> queues;
    std::mutex mutexQueues;
    std::condition_variable cond_insertedElement;
    std::condition_variable empty;
    uint32_t queuedElements;

    // RANDOM:
    std::hash<std::string> hash_fn;
    std::mutex mutexRandom;
    std::minstd_rand0 lRand;
};

}

}}


// TODO: Failed task what to do?, using

