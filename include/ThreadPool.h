#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @brief 线程池，用来管理执行的测试任务
 *
 */
class ThreadPool {
  public:
    /**
     * @brief 构造函数
     *
     * @param threads 指定线程池中线程数量，默认使用硬件并发数
     */
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());

    /**
     * @brief 任务入队方法：将任意可调用对象和参数加入任务队列
     *
     * @tparam F 可调用对象类型
     * @tparam Args 参数包类型
     * @return 与任务结果关联的std::future
     */
    template <class F, class... Args> auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F(Args...)>::type> {  // 使用typename明确告诉编译器::type是类型
        // 推导任务返回类型
        using return_type = typename std::invoke_result<F(Args...)>::type;
        // 创建packaged_task来包装任务，用于获取future
        // 使用shared_ptr来实现堆任务对象的共享所有权
        auto task = std::make_shared<std::packaged_task<return_type()>>(
          // 使用bind完美转发参数
          // 将函数 f 与参数 args... 绑定，从而生成了一个新的无参数可调用对象
          std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        // 获取与任务关联的future对象
        std::future<return_type> res = task->get_future();
        {
            // 临界区开始
            std::unique_lock<std::mutex> lock(queue_mutex);
            // 如果线程池已停止，禁止新任务加入
            if(stop) throw std::runtime_error("enqueue on stopped ThreadPool");

            // 将任务包装为void()类型的lambda表达式加入队列
            tasks.emplace([task]() {
                (*task)();  // 执行packaged_task
            });
        }  // 释放锁

        // 通知一个线程有新任务到来
        condition.notify_one();
        return res;  // 返回future对象
    }

    /**
     * @brief Destroy the Thread Pool object
     */
    ~ThreadPool();

  private:
    std::vector<std::thread> workers;         // 工作线程容器
    std::queue<std::function<void()>> tasks;  // 任务队列

    std::mutex queue_mutex;             // 保护任务队列的互斥锁
    std::condition_variable condition;  // 任务通知条件变量
    bool stop;                          // 线程池停止标志位
};