#include <filesystem>
#include <iostream>

#include "ConcurrentResultWriter.h"
#include "TestExecutor.h"
#include "TestPreprocess.h"
#include "ThreadPool.h"

// IntersectorCoroutineBatchTests* : IntersectorGeometryCoroutineBatchTests* : Intersector_SliceGeneratorBatchTests*:
// Spd_IntersectorSpdPrimtiveGeneratorBatchTests* : IntersectorSpdModelGeneratorBatchTests* : IntersectorSpdBoatGeneratorBatchTests*

int main() {
    std::string exe_path = "D:/GithubProject/GME-Test/GME-Build/Release/tests_acis.exe";
    std::string gtest_filter =
      "IntersectorCoroutineBatchTests*:IntersectorGeometryCoroutineBatchTests*:Intersector_SliceGeneratorBatchTests*:Spd_IntersectorSpdPrimtiveGeneratorBatchTests*:IntersectorSpdModelGeneratorBatchTests*:IntersectorSpdBoatGeneratorBatchTests*";
    int thread_num = std::thread::hardware_concurrency();
    int time_out_sec = 30;

    std::filesystem::path parent_dir = std::filesystem::path(exe_path).parent_path();
    std::filesystem::path out_path = parent_dir / "output";
    std::filesystem::create_directory(out_path);
    std::string test_list_file = out_path.string() + "/test_name.txt";
    std::string result_file = out_path.string() + "/result.txt";

    TestPreprocess tp(gtest_filter, exe_path, test_list_file);
    tp.GetTests();

    // 读取测试列表
    std::vector<std::string> test_names;
    {
        std::ifstream infile(test_list_file);
        std::string line;
        while(std::getline(infile, line)) {
            if(!line.empty()) test_names.push_back(line);
        }
    }

    // 初始化组件
    ThreadPool pool(thread_num);
    ConcurrentResultWriter writer(result_file);
    TestExecutor executor(pool, exe_path, writer, time_out_sec);

    writer.total_ = static_cast<int>(test_names.size());

    auto start_time = std::chrono::steady_clock::now();
    // 提交任务
    for(auto const& name: test_names) {
        executor.SubmitTest(name);
    }

    // 等待任务完成
    while(writer.completed_ < test_names.size()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        writer.PrintProgress();
    }
    auto total_time = std::chrono::steady_clock::now() - start_time;
    auto seconds_float = std::chrono::duration<double>(total_time).count();
    std::cout << "\nAll tests completed!\n";
    std::cout << "总耗时：" << seconds_float << "秒";

    return 0;
}