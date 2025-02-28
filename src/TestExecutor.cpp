#include "TestExecutor.h"

#include <WTypesbase.h>

#include "ConcurrentResultWriter.h"

/**
 * @brief Construct a new Test Executor object
 *
 * @param pool
 * @param exe_path
 * @param writer
 * @param timeout_sec
 */
TestExecutor::TestExecutor(ThreadPool& pool, std::string const& exe_path, ConcurrentResultWriter& writer, int timeout_sec): pool_(pool), exe_path_(exe_path), writer_(writer), time_out_(timeout_sec) {
}

/**
 * @brief 向进程池提交一组测例，交由一个线程执行
 *
 * @param test_names
 */
void TestExecutor::SubmitTestBatch(std::vector<std::string> const& test_names) {
    // std::cout << "Submitting batch of " << test_names.size() << " tests\n";
    pool_.enqueue([this, test_names] {
        auto cmd = BuildCommand(test_names);
        auto result = ExecuteTest(cmd, test_names);

        // 处理完成, 超时, 中断的测例
        writer_.completed_ += static_cast<int>(result.complete_tests.size() + result.timed_out_tests.size() + result.skipped_tests.size() + result.interrupted_tests.size());
        writer_.AddResult(result);

        // 记录处理结果
        // std::cout << "Batch completed. Complete: " << result.complete_tests.size() << "Skipped: " << result.skipped_tests.size() << ", Timed out: " << result.timed_out_tests.size() << ", Interrupted: " << result.interrupted_tests.size()
        //           << ", Remaining: " << result.remaining_tests.size() << std::endl;
        // 重新提交剩余测例
        if(!result.remaining_tests.empty()) {
            SubmitTestBatch(result.remaining_tests);
        }
    });
}

std::string TestExecutor::BuildCommand(std::vector<std::string> const& test_names) {
    std::string filter;
    for(auto& test_name: test_names) filter += test_name + ":";
    return exe_path_ + " --gtest_filter=" + filter;
}

/**
 * @brief 执行任务
 *
 * @param command
 * @return std::pair<int, std::string>
 */
ExecuteResult TestExecutor::ExecuteTest(std::string const& command, std::vector<std::string> const& test_names) {
    SECURITY_ATTRIBUTES sa;             // 定义对象 (如管道，文件，句柄) 的安全属性和继承属性
    sa.nLength = sizeof(sa);            // 必须显示这样设置
    sa.bInheritHandle = true;           // 表示子句柄可以被继承
    sa.lpSecurityDescriptor = nullptr;  // 指向描述安全符的指针，nullptr表示使用默认安全设置

    HANDLE hOutputRead, hOutputWrite;
    CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0);            // 创建管道读取端，写入端句柄，设置句柄的继承性和安全性，0表示管道默认大小
    SetHandleInformation(hOutputRead, HANDLE_FLAG_INHERIT, 0);  // 禁止读取端管道被继承

    STARTUPINFOA si = {sizeof(si)};     // 定义新进程的启动参数，包括窗口属性，标准输入输出句柄等
    si.dwFlags = STARTF_USESTDHANDLES;  // 当设置 dwFlags 为 STARTF_USESTDHANDLES 时，系统会使用 hStdInput、hStdOutput、hStdError 三个字段的值作为子进程的标准输入、输出、错误句柄。
    si.hStdOutput = hOutputWrite;
    si.hStdError = hOutputWrite;

    PROCESS_INFORMATION pi;  // 存储新进程的信息
    // 为当前进程创建一个新的子进程
    bool sucess = CreateProcessA(nullptr,                             // lpApplicationName: 指定要执行的可执行文件路径，nullptr时会从lpCommandLine解析
                                 const_cast<LPSTR>(command.c_str()),  // lpCommandLine: 完整的命令行字符串，包含可执行文件路径和参数
                                 nullptr,                             // lpProcessAttributes: 定义新进程的安全属性和继承性 (通常不需要特殊设置)
                                 nullptr,                             // lpThreadAttributes: 定义新进程主线程的安全属性和继承性（通常不需要特殊设置）
                                 true,                                // bInheritHandles: 是否允许子进程继承父进程的可继承句柄
                                 CREATE_NO_WINDOW,                    // dwCreationFlags: 不显示控制台窗口
                                 nullptr,                             // lpEnvironment: 指定子进程的环境变量，nullptr表示继承父进程
                                 nullptr,                             // lpCurrentDirectory: 指定子进程的工作目录，nullpte表示继承父进程
                                 &si,                                 // lpStartupInfo: 指向 STARTUPINFOA 结构体的指针，包含标准句柄重定向等配置
                                 &pi                                  // lpProcessInformation: 接收新进程的句柄和ID，后续用于管理进程
    );

    if(!sucess) {
        CloseHandle(hOutputRead);
        throw std::runtime_error("CreateProcess failed");
    }

    CloseHandle(hOutputWrite);  // 父进程无需写端，立即关闭

    std::vector<std::string> remaining_tests = test_names;
    std::vector<std::pair<std::string, std::string>> completed_tests;
    std::vector<std::string> skipped_tests;
    std::vector<std::string> timed_out_tests;
    std::vector<std::string> interrupted_tests;
    std::string current_test;
    auto start_time = std::chrono::steady_clock::now();

    // 异步读取输出
    char buffer[4096];
    DWORD bytesRead;  // 保存ReadFile实际读取的字节数
    std::string output;
    bool process_exited = false;

    while(true) {
        // 非阻塞检查子进程是否已经退出
        DWORD exit_code;
        if(GetExitCodeProcess(pi.hProcess, &exit_code) && exit_code != STILL_ACTIVE) {
            process_exited = true;
            // 检测异常退出 (非正常结束且非超时终止)
            if(exit_code != 0 && timed_out_tests.empty()) {
                if(!current_test.empty()) {
                    interrupted_tests.push_back(current_test);
                    auto it = std::find(remaining_tests.begin(), remaining_tests.end(), current_test);
                    if(it != remaining_tests.end()) {
                        remaining_tests.erase(it);
                    }
                }
            }
        }

        // 非阻塞读取，首先检查管道中是否有数据可以读取
        while(PeekNamedPipe(hOutputRead, nullptr, 0, nullptr, &bytesRead, nullptr) && bytesRead > 0) {
            // 从管道中读取数据
            ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, nullptr);
            output.append(buffer, bytesRead);
        }

        // 解析输出
        std::istringstream iss(output);
        std::string line;
        while(std::getline(iss, line)) {
            if(line[line.size() - 1] == '\r') line.erase(line.size() - 1);
            if(line.find("[ RUN      ]") != std::string::npos) {
                current_test = line.substr(13);
                start_time = std::chrono::steady_clock::now();
            } else if(line.find("[       OK ]") != std::string::npos || line.find("[  FAILED  ]") != std::string::npos) {
                if(!current_test.empty()) {
                    auto it = std::find(remaining_tests.begin(), remaining_tests.end(), current_test);
                    if(it != remaining_tests.end()) {
                        completed_tests.push_back({current_test, line.find("[       OK ]") != std::string::npos ? "Passed" : "Failed"});
                        remaining_tests.erase(it);
                    }
                }
                current_test.clear();
            } else if(line.find("[  SKIPPED ]") != std::string::npos) {
                if(!current_test.empty()) {
                    auto it = std::find(remaining_tests.begin(), remaining_tests.end(), current_test);
                    if(it != remaining_tests.end()) {
                        skipped_tests.push_back(current_test);
                        remaining_tests.erase(it);
                    }
                }
                current_test.clear();
            }
        }

        // 检查超时
        if(!current_test.empty()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            if(elapsed >= time_out_) {
                TerminateProcess(pi.hProcess, 1);
                timed_out_tests.push_back(current_test);
                auto it = std::find(remaining_tests.begin(), remaining_tests.end(), current_test);
                if(it != remaining_tests.end()) {
                    remaining_tests.erase(it);
                }
                current_test.clear();
                break;
            }
        }

        if(process_exited) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 读取剩余输出
    while(ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);

    return {0, output, completed_tests, skipped_tests, timed_out_tests, remaining_tests, interrupted_tests};
}