#pragma once

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#    define POPEN _popen
#    define PCLOSE _pclose
#else
#    define POPEN popen
#    define PCLOSE pclose
#endif

class TestPreprocess {
  public:
    /**
     * @brief Construct a new Test Preprocess object
     *
     * @param gtest_filter
     * @param out_file 输出文件
     */
    explicit TestPreprocess(std::string const& gtest_filter, std::string const& exe_path, std::string const& out_file = "test_name.txt");

    /**
     * @brief Get the Tests
     */
    void GetTests();

    /**
     * @brief 获得全部的tests name
     *
     * @return std::string 存储全部的tests name
     */
    std::string GetOriginRes();

    /**
     * @brief 解析测例
     *
     * @param output
     * @return std::vector<std::string>
     */
    std::vector<std::string> ParseTestOutput(std::string const& output);

    /**
     * @brief 生成正则过滤表达式
     *
     * @param filter
     * @return std::vector<std::regex>
     */
    std::vector<std::regex> GenerateFilters(std::string const& filter);

    /**
     * @brief 过滤并写入结果
     *
     * @param tests
     * @param filters
     */
    void FilterAndWriteResults(std::vector<std::string>& tests, std::vector<std::regex> const& filters);

  private:
    std::string gtest_filter_;
    std::string exe_path_;
    std::string out_file_;
};