#pragma once

#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>

struct TestTesult {
    std::string name;
    std::string status;
    std::string output;
};

class ConcurrentResultWriter {
  public:
    /**
     * @brief Construct a new Concurrent Result Writer object
     *
     * @param file_name
     */
    ConcurrentResultWriter(std::string const& file_name);

    /**
     * @brief 输出结果
     *
     * @param result
     */
    void AddResult(TestTesult const& result);

    /**
     * @brief 打印进度状态
     *
     */
    void PrintProgress();

    std::mutex mtx_;
    std::ofstream outfile_;
    std::atomic<int> total_;
    std::atomic<int> completed_;
};
