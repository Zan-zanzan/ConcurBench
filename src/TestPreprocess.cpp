#include "TestPreprocess.h"

/**
 * @brief Construct a new Test Preprocess object
 *
 * @param gtest_filter
 * @param out_file 输出文件
 */
TestPreprocess::TestPreprocess(std::string const& gtest_filter, std::string const& exe_path, std::string const& out_file): gtest_filter_(gtest_filter), exe_path_(exe_path), out_file_(out_file) {
}

/**
 * @brief Get the Tests
 */
void TestPreprocess::GetTests() {
    // Step1: 获取原始列表
    std::string raw_output = GetOriginRes();

    // Step2: 解析列表
    std::vector<std::string> all_tests = ParseTestOutput(raw_output);

    // Step3: 生成正则过滤表达式
    std::vector<std::regex> filters = GenerateFilters(gtest_filter_);

    // Step4: 执行过滤并写入文件
    FilterAndWriteResults(all_tests, filters);
}

/**
 * @brief 获得全部的tests name
 *
 * @return std::string 存储全部的tests name
 */
std::string TestPreprocess::GetOriginRes() {
    const std::string command = exe_path_ + " --gtest_list_tests";

    // 跨平台管道读取
    FILE* pipe = POPEN(command.c_str(), "r");
    if(!pipe) throw std::runtime_error("popen() failed");

    // 读取输出到缓冲区
    char buffer[128];
    std::string result;
    while(fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    if(PCLOSE(pipe) != 0) throw std::runtime_error("pclose() failed");

    return result;
}

/**
 * @brief 解析测例
 *
 * @param output
 * @return std::vector<std::string>
 */
std::vector<std::string> TestPreprocess::ParseTestOutput(std::string const& output) {
    std::vector<std::string> tests;
    std::istringstream iss(output);
    std::string line;
    std::string current_suite;

    while(std::getline(iss, line)) {
        // 去除行尾换行符
        if(!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        // 识别套件 以 . 结尾
        if(line.find('.') == line.size() - 1) {
            current_suite = line.substr(0, line.size() - 1);
        } else if(!current_suite.empty()) {
            // 处理测试用例行 处理缩进
            size_t start = line.find_first_not_of(" ");
            if(start != std::string::npos) {
                tests.push_back(current_suite + "." + line.substr(start));
            }
        }
    }

    return tests;
}

/**
 * @brief 生成正则过滤表达式
 *
 * @param filter
 * @return std::vector<std::regex>
 */
std::vector<std::regex> TestPreprocess::GenerateFilters(std::string const& filter) {
    std::vector<std::regex> filters;

    // 分割多个过滤条件，以 : 分割
    size_t pos = 0;
    while(pos < filter.size()) {
        size_t end = filter.find(':', pos);
        if(end == std::string::npos) end = filter.size();

        std::string pattern = filter.substr(pos, end - pos);

        // 转换通配符语法到正则表达式
        std::string regex_pattern;
        for(char c: pattern) {
            switch(c) {
                case '*':
                    regex_pattern += ".*";
                    break;
                case '?':
                    regex_pattern += '.';
                    break;
                case '.':
                    regex_pattern += "\\.";
                    break;
                default:
                    regex_pattern += c;
            }
        }

        // 保证匹配完整字符串
        regex_pattern = "^" + regex_pattern + "$";

        try {
            filters.emplace_back(regex_pattern, std::regex::icase);
        } catch(const std::regex_error& e) {
            throw std::runtime_error("Invalid regex pattern: " + regex_pattern + "\n" + e.what());
        }

        if(end == filter.size())
            pos = end;
        else
            pos = end + 1;
    }

    return filters;
}

/**
 * @brief 过滤并写入结果
 *
 * @param tests
 * @param filters
 */
void TestPreprocess::FilterAndWriteResults(std::vector<std::string>& tests, std::vector<std::regex> const& filters) {
    std::ofstream outfile(out_file_);
    if(!outfile.is_open()) throw std::runtime_error("Cannot open output file: " + out_file_);

    for(auto const& test: tests) {
        // 检查是否匹配任意过滤规则
        bool matched = std::any_of(filters.begin(), filters.end(), [&test](std::regex const& re) { return std::regex_match(test, re); });

        if(matched) outfile << test << "\n";
    }

    outfile.close();
}