#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "ThreadPool.h"

class ConcurrentResultWriter;

struct ExecuteResult {
    int exit_code;
    std::string output;
    std::vector<std::pair<std::string, std::string>> complete_tests;
    std::vector<std::string> skipped_tests;
    std::vector<std::string> timed_out_tests;
    std::vector<std::string> remaining_tests;
    std::vector<std::string> interrupted_tests;
};

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
     * @brief 向进程池提交一组测例，交由一个线程执行
     *
     * @param test_names
     */
    void SubmitTestBatch(std::vector<std::string> const& test_names);

    /**
     * @brief 生成command
     *
     * @param test_names
     * @return std::string
     */
    std::string BuildCommand(std::vector<std::string> const& test_names);

    /**
     * @brief 执行任务
     *
     * @param command
     * @return ExecuteResult
     */
    ExecuteResult ExecuteTest(std::string const& command, std::vector<std::string> const& test_names);

  private:
    ThreadPool& pool_;
    std::string exe_path_;
    ConcurrentResultWriter& writer_;
    int time_out_;
};