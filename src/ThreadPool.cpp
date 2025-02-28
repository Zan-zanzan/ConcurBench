#include "ThreadPool.h"
/**
 * @brief 构造函数
 *
 * @param threads 指定线程池中线程数量，默认使用硬件并发数
 */
ThreadPool::ThreadPool(size_t threads): stop(false) {
    // 创建指定数量的工作线程
    for(size_t i = 0; i < threads; i++) {
        // [this]() -> void {}的简写
        workers.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);

                    // void wait(std::unique_lock<std::mutex> & lock, Predicate pred);
                    // 阻塞当前线程，释放锁，直到被 notify 唤醒后重新获取锁
                    // 传入lambda函数以检测虚假唤醒，若线程池已经停止或者任务列表为空，则不会唤醒进程
                    // 等价于
                    // while(!pred()) wait(lock); 若条件为假则持续阻塞
                    // while(!this->stop && this->tasks.empty()) wait(lock) 只有进入循环才会阻塞自己
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                    // 检查终止条件：当停止标志位为真且任务队列为空的时候退出线程
                    if(this->stop && this->tasks.empty()) return;

                    // 从任务队列中获取任务
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();  // 执行获取到的任务
            }
        });
    }
}

/**
 * @brief Destroy the Thread Pool object
 */
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();                           // 唤醒所有线程
    for(std::thread& worker: workers) worker.join();  // 主线程（调用线程）将阻塞直到所有工作线程执行完毕
}