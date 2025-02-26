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
void ConcurrentResultWriter::AddResult(TestTesult const& result) {
    std::lock_guard<std::mutex> lock(mtx_);
    outfile_ << result.name << " : " << result.status << "\n";
    outfile_.flush();
    ++completed_;
}

/**
 * @brief 打印进度状态
 *
 */
void ConcurrentResultWriter::PrintProgress() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << "\rProgress: " << completed_ << "/" << total_;
}