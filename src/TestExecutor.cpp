#include "TestExecutor.h"

#include <WTypesbase.h>

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
 * @brief 向进程池提交任务
 *
 * @param test_name
 */
void TestExecutor::SubmitTest(std::string const& test_name) {
    pool_.enqueue([this, test_name]() -> void {
        TestTesult result;
        result.name = test_name;
        auto cmd = BuildCommand(test_name);
        auto [exit_code, output] = ExecuteTest(cmd);

        result.status = (exit_code == 0) ? "Passed" : "Failed";
        result.output = output;

        writer_.AddResult(result);
        writer_.PrintProgress();
    });
}

/**
 * @brief 生成command
 *
 * @param test_name
 * @return std::string
 */
std::string TestExecutor::BuildCommand(std::string const& test_name) {
    return exe_path_ + " --gtest_filter=" + test_name;
}

/**
 * @brief 执行任务
 *
 * @param command
 * @return std::pair<int, std::string>
 */
std::pair<int, std::string> TestExecutor::ExecuteTest(std::string const& command) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = true;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hOutputRead, hOutputWrite;
    CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0);
    SetHandleInformation(hOutputRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hOutputWrite;
    si.hStdError = hOutputWrite;

    PROCESS_INFORMATION pi;
    bool sucess = CreateProcessA(nullptr, const_cast<LPSTR>(command.c_str()), nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if(!sucess) {
        throw std::runtime_error("CreateProcess failed");
    }

    CloseHandle(hOutputWrite);

    // 异步读取输出
    std::string output;
    char buffer[4096];
    DWORD bytesRead;

    auto start = std::chrono::steady_clock::now();
    while(true) {
        if(WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) break;

        // 超时处理
        auto elapsed = std::chrono::steady_clock::now() - start;
        if(elapsed > std::chrono::seconds(time_out_)) {
            TerminateProcess(pi.hProcess, 1);
            throw std::runtime_error("Test timeout");
        }

        // 非阻塞读取
        if(PeekNamedPipe(hOutputRead, nullptr, 0, nullptr, &bytesRead, nullptr) && bytesRead > 0) {
            ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, nullptr);
            output.append(buffer, bytesRead);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 读取剩余输出
    while(ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);
    return {exit_code, output};
}