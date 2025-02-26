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

        writer_.completed_++;
        // writer_.AddResult(result);
        // writer_.PrintProgress();
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
        throw std::runtime_error("CreateProcess failed");
    }

    CloseHandle(hOutputWrite);  // 父进程无需写端，立即关闭

    // 异步读取输出
    std::string output;
    char buffer[4096];
    DWORD bytesRead;  // 保存ReadFile实际读取的字节数

    auto start = std::chrono::steady_clock::now();
    while(true) {
        // 非阻塞检查子进程是否已经退出
        if(WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) break;

        // 超时处理
        auto elapsed = std::chrono::steady_clock::now() - start;
        if(elapsed > std::chrono::seconds(time_out_)) {
            TerminateProcess(pi.hProcess, 1);
            throw std::runtime_error("Test timeout");
        }

        // 非阻塞读取，首先检查管道中是否有数据可以读取
        if(PeekNamedPipe(hOutputRead, nullptr, 0, nullptr, &bytesRead, nullptr) && bytesRead > 0) {
            // 从管道中读取数据
            ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, nullptr);
            output.append(buffer, bytesRead);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 减少cpu占用
    }

    // 读取剩余输出, 确保子进程退出后管道中的所有数据都被读取
    while(ReadFile(hOutputRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    DWORD exit_code;
    // 获取进程退出码
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);
    return {exit_code, output};
}