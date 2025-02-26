#pragma once

#include <chrono>

#include "ConcurrentResultWriter.h"
#include "ThreadPool.h"

class TestExecutor {
  public:
    /**
     * @brief Construct a new Test Executor object
     *
     * @param pool
     * @param exe_path
     * @param writer
     * @param timeout_sec
     */
    TestExecutor(ThreadPool& pool, std::string const& exe_path, ConcurrentResultWriter& writer, int timeout_sec = 30);

    /**
     * @brief 向进程池提交任务
     *
     * @param test_name
     */
    void SubmitTest(std::string const& test_name);

    /**
     * @brief 生成command
     *
     * @param test_name
     * @return std::string
     */
    std::string BuildCommand(std::string const& test_name);

    /**
     * @brief 执行任务
     *
     * @param command
     * @return std::pair<int, std::string>
     */
    std::pair<int, std::string> ExecuteTest(std::string const& command);

  private:
    ThreadPool& pool_;
    std::string exe_path_;
    ConcurrentResultWriter& writer_;
    int time_out_;
};