# GTest 并发测试执行框架

![C++17](https://img.shields.io/badge/C++-17-blue.svg)
![Windows Build](https://img.shields.io/badge/Platform-Windows-0078d7.svg)
![MIT License](https://img.shields.io/badge/License-MIT-green.svg)

专业的并行化Google Test执行引擎，为大规模测试套件提供高效的执行调度和资源管理。本框架通过创新的动态批处理算法和智能超时控制机制，显著提升测试执行效率，同时保证测试结果的完整性和可靠性。

## 🌟 核心特性

- ​**智能并发调度**
  - 基于硬件并发度的自适应线程池
  - 动态批处理策略（DBP）优化任务分配
  - 负载均衡的任务窃取机制

- ​**鲁棒性保障**
  - 基于Windows Job Object的进程隔离
  - 分层式超时控制（测试级/批次级）
  - 异常中断恢复与重试机制

- ​**高级诊断能力**
  - 实时执行状态监控仪表盘
  - 细粒度测试结果分类（通过/失败/超时/中断）
  - 结构化结果日志与执行轨迹记录

- ​**企业级扩展**
  - 支持分布式测试节点协同
  - 可插拔的结果导出模块（JSON/XML/CSV）
  - Prometheus格式的运行时指标输出

## 🧩 系统架构

```mermaid
graph TD
    A[主控制器] --> B[测试预处理器]
    A --> C[线程池管理器]
    B --> D[正则过滤引擎]
    C --> E[动态批处理调度器]
    E --> F[测试执行代理]
    F --> G[进程隔离沙箱]
    G --> H[结果分析器]
    H --> I[并发写入器]
