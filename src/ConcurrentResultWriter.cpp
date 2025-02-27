#include "ConcurrentResultWriter.h"

/**
 * @brief Construct a new Concurrent Result Writer object
 *
 * @param file_name
 */
ConcurrentResultWriter::ConcurrentResultWriter(std::string const& file_name): outfile_(file_name, std::ios::app), completed_(false) {
}

/**
 * @brief 输出结果
 *
 * @param result
 */
void ConcurrentResultWriter::AddResult(TestResult const& result) {
    std::lock_guard<std::mutex> lock(mtx_);
    outfile_ << result.name << " : " << result.status << "\n";
    outfile_.flush();
}

void ConcurrentResultWriter::AddResult(std::string const& result) {
    std::lock_guard<std::mutex> lock(mtx_);
    // 处理result
    std::stringstream ss(result);
    std::string line;
    // 分隔每个测例的测试结果
    std::vector<std::vector<std::string>> tests_result;
    int test_idx = -1;
    bool useful_info = false;
    while(std::getline(ss, line)) {
        if(line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        if(line.find("[ RUN      ]") != std::string::npos) {
            useful_info = true;
            test_idx++;
            tests_result.push_back(std::vector<std::string>());
        }
        if(line.find("Global test environment tear-down") != std::string::npos) {
            useful_info = false;
        }
        if(useful_info) {
            tests_result[test_idx].push_back(line);
        }
    }

    std::vector<TestResult> output_res;
    const int name_idx = 13;
    for(int i = 0; i < tests_result.size(); i++) {
        TestResult res;
        bool passed = false;
        bool failed = false;
        for(int j = 0; j < tests_result[0].size(); j++) {
            auto const& line_str = tests_result[i][j];
            if(j == 0)
                res.name = line_str.substr(name_idx);
            else if(line_str.find("[       OK ]") != std::string::npos)
                passed = true;
            else if(line_str.find("[  FAILED  ]") != std::string::npos)
                failed = true;
        }
        if(passed)
            res.status = "Passed";
        else if(failed)
            res.status = "Failed";
        else
            res.status = "中断";
        output_res.push_back(res);
    }
    for(auto const& res: output_res) outfile_ << res.name << " " << res.status << "\n";
}

/**
 * @brief 打印进度状态
 *
 */
void ConcurrentResultWriter::PrintProgress() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << "\rProgress: " << completed_ << "/" << total_;
}