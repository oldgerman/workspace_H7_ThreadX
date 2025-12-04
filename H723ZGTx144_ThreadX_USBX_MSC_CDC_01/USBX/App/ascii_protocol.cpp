#include "stdint.h"
#include <memory>  // std::unique_ptr依赖此头文件
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdarg.h>  // 可变参数头文件

#include "main.h"
#include "ux_device_cdc_acm.h"
#include "ascii_protocol.h"

#include "app_demo_sd_filex.h"

StreamSink usb_response_channel;  // USB回应通道

// 复用你的Respond函数（修正后版本）
static void Respond(StreamSink& _responseChannel, bool _isError, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    char format[256];

    if (_isError) {
        snprintf(format, sizeof(format), "ERROR: %s\r\n", fmt);
    } else {
        snprintf(format, sizeof(format), "%s\r\n", fmt);
    }
    vsnprintf(buf, sizeof(buf), format, args);
    _responseChannel.send("%s", buf); // 调用send接口
    va_end(args);
}

// 占位：复用你的RespondIsrStackUsageInWords函数
static void RespondIsrStackUsageInWords(StreamSink& _responseChannel) {
    Respond(_responseChannel, false, "ISR Stack Usage: 123 words");
}

// 占位：PSRAM测试函数
void PSRAM_Test(uint32_t base) { /* 你的逻辑 */ }


// 命令执行函数类型：接收命令体（前缀后的内容）、回应通道，无返回值
using CmdExecFunc = std::function<void(const std::string& cmdBody, StreamSink& response)>;

// 命令基类：定义通用接口
class CommandBase {
public:
    // 构造函数：前缀（必填）、命令名（必填）、描述（可选，默认孔）、辅助参数（可选，默认空）
    CommandBase(char prefix, const std::string& cmdName, const std::string& desc = "", const std::string& auxParam = "")
        : m_prefix(prefix), m_cmdName(cmdName), m_desc(desc), m_auxParam(auxParam) {}

    virtual ~CommandBase() = default;

    // 核心接口：匹配命令（前缀+命令名）
    virtual bool match(char prefix, const std::string& cmdBody) const {
        if (prefix != m_prefix) return false;
        // 命令名为空（如'f'前缀）→ 只要前缀匹配即可；否则检查cmdBody包含命令名
        return m_cmdName.empty() || (cmdBody.find(m_cmdName) != std::string::npos);
    }

    // 核心接口：执行命令
    virtual void execute(const std::string& cmdBody, StreamSink& response) = 0;

    // 获取属性（供注册器使用）
    char getPrefix() const { return m_prefix; }
    // 获取命令名
    std::string getCmdName() const { return m_cmdName; }
    // 获取命令描述
    std::string getDesc() const { return m_desc; }

protected:
    char m_prefix;          // 命令前缀（f/^/#/$等）
    std::string m_cmdName;  // 命令名  （STOP/GETFLOAT/TEST_PSRAM等）
    std::string m_desc;     // 命令描述（可选，空字符串默认）
    std::string m_auxParam; // 辅助参数（可选，空字符串默认）
};

// 通用命令子类：绑定执行函数，适配所有命令类型
class GenericCommand : public CommandBase {
public:
    // 构造函数：前缀+命令名+执行函数+描述（可选）+辅助参数（可选）
    GenericCommand(char prefix, const std::string& cmdName, const CmdExecFunc& execFunc, const std::string& desc = "", const std::string& auxParam = "")
        : CommandBase(prefix, cmdName, desc, auxParam), m_execFunc(execFunc) {}

    // 重写执行接口：调用绑定的执行函数
    void execute(const std::string& cmdBody, StreamSink& response) override {
        if (m_execFunc) {
            m_execFunc(cmdBody, response);
        } else {
            Respond(response, true, "No exec function bound");
        }
    }

private:
    CmdExecFunc m_execFunc; // 绑定的执行函数
};


class CommandRegistry {
public:
    // 单例：全局唯一实例
    static CommandRegistry& getInstance() {
        static CommandRegistry instance;
        return instance;
    }

    // 注册命令：接收前缀+命令名+执行函数+辅助参数（可选）
    // 修改registerCommand方法，替换std::make_unique
    void registerCommand(
    		char prefix,
    		const std::string& cmdName,
			const CmdExecFunc& execFunc,
			const std::string& desc = "",
			const std::string& auxParam = "") {
        // 构造对象时传入描述
        m_cmdMap[prefix][cmdName] = std::unique_ptr<GenericCommand>(
            new GenericCommand(prefix, cmdName, execFunc, desc, auxParam)
        );
    }

    // 解析并执行命令（替代原有OnAsciiCmd的if-else逻辑）
    void parseAndExecute(const char* _cmd, size_t _len, StreamSink& response) {
        if (_len == 0 || _cmd == nullptr) {
            Respond(response, true, "Empty command");
            return;
        }

        // 拆分前缀和命令体（如"$TEST_PSRAM" → 前缀'$'，命令体"TEST_PSRAM"）
        char prefix = _cmd[0];
        std::string cmdBody(_cmd + 1, _len - 1); // 前缀后的所有内容

        // 查找前缀对应的命令集合
        auto prefixIt = m_cmdMap.find(prefix);
        if (prefixIt == m_cmdMap.end()) {
            Respond(response, true, "Unknown prefix: %c", prefix);
            return;
        }

        // 遍历该前缀下的所有命令，匹配则执行
        bool cmdMatched = false;
        // 替换结构化绑定为迭代器遍历（C++11/14支持）
        for (auto& cmdPair : prefixIt->second) {
            // cmdPair是map的键值对：first=cmdName，second=cmdPtr（unique_ptr<CommandBase>）
            std::unique_ptr<CommandBase>& cmdPtr = cmdPair.second;

            if (cmdPtr->match(prefix, cmdBody)) {
                cmdPtr->execute(cmdBody, response);
                cmdMatched = true;
                break; // 匹配到第一个即执行（避免多命令匹配）
            }
        }

        if (!cmdMatched) {
            Respond(response, true, "Unknown command for prefix %c: %s", prefix, cmdBody.c_str());
        }
    }

    // 获取所有已注册命令的信息（返回格式："前缀 命令名：描述"）
    std::vector<std::string> getAllCmdInfo() {
        std::vector<std::string> cmdList;
        // 遍历所有前缀（替换C++17结构化绑定，用迭代器+pair）
        for (auto& prefixPair : m_cmdMap) {
            // prefixPair是pair<char, map<string, unique_ptr<CommandBase>>>
            char prefix = prefixPair.first;          // 取前缀字符
            std::map<std::string, std::unique_ptr<CommandBase>>& cmdMap = prefixPair.second; // 取该前缀下的命令map

            // 遍历该前缀下的所有命令（原有兼容写法，无需改）
            for (auto& cmdPair : cmdMap) {
                const std::string& cmdName = cmdPair.first;
                CommandBase* cmdPtr = cmdPair.second.get();

                // 拼接帮助信息：比如 "^ STOP：停止操作"、"$ ISR_STACK：查看ISR栈使用"
                std::string info = std::string(1, prefix) + " " + cmdName + "：" + cmdPtr->getDesc();
                cmdList.push_back(info);
            }
        }
        return cmdList;
    }

    // 禁止拷贝（单例）
    CommandRegistry(const CommandRegistry&) = delete;
    CommandRegistry& operator=(const CommandRegistry&) = delete;

private:
    CommandRegistry() = default; // 私有构造

    // 命令存储：map<prefix, map<cmdName, unique_ptr<CommandBase>>>
    std::map<char, std::map<std::string, std::unique_ptr<CommandBase>>> m_cmdMap;
};



// 初始化命令注册， 可在 USBD_CDC_ACM_Activate () 调用以注册命令（程序启动时调用一次）
void initCommandRegistry() {
    // 静态标志，确保只初始化一次
    static bool initialized = false;
    if (initialized) {
        return;  // 已经初始化过，直接返回
    }
    initialized = true;

    auto& registry = CommandRegistry::getInstance();

    // ========== 1. 先注册帮助命令（前缀 'h'） ==========
    registry.registerCommand('h', "", [](const std::string&, StreamSink& response) {
        // 获取所有已注册命令的信息
        auto cmdList = CommandRegistry::getInstance().getAllCmdInfo();

        // 先输出标题
        Respond(response, false, "===== 支持的命令列表 =====");
        // 遍历输出每个命令
        if (cmdList.empty()) {
            Respond(response, false, "暂无注册命令");
        } else {
            for (const std::string& info : cmdList) {
                Respond(response, false, info.c_str());
            }
        }
        // 输出结束符
        Respond(response, false, "==========================");
    }, "查看所有支持的命令", ""); // 把desc放到第四个参数，auxParam留空

    // ========== 2. 补充所有命令的描述字段 ==========
    // 1. 注册前缀'f'的命令：解析浮点数
    registry.registerCommand('f', "", [](const std::string& cmdBody, StreamSink& response) {
        float value;
        if (sscanf(cmdBody.c_str(), "%f", &value) == 1) {
            Respond(response, false, "Got float: %f", value);
        } else {
            Respond(response, true, "Invalid float format: %s", cmdBody.c_str());
        }
    }, "解析浮点数"); // 补充描述

    // 2. 注册前缀'^'的命令：STOP/START/DISABLE
    registry.registerCommand('^', "STOP", [](const std::string&, StreamSink& response) {
        Respond(response, false, "Stopped ok");
    }, "停止操作"); // 补充描述

    registry.registerCommand('^', "START", [](const std::string&, StreamSink& response) {
        Respond(response, false, "Started ok");
    }, "启动操作"); // 补充描述

    registry.registerCommand('^', "DISABLE", [](const std::string&, StreamSink& response) {
        Respond(response, false, "Disabled ok");
    }, "禁用操作"); // 补充描述

    // 3. 注册前缀'#'的命令：GETFLOAT/GETINT/CMDMODE
    registry.registerCommand('#', "GETFLOAT", [](const std::string&, StreamSink& response) {
        Respond(response, false, "ok %.2f %.2f %.2f %.2f %.2f %.2f",
                1.23f, 4.56f, 7.89f, 9.87f, 6.54f, 3.21f);
    }, "获取浮点数组"); // 补充描述

    registry.registerCommand('#', "GETINT", [](const std::string&, StreamSink& response) {
        Respond(response, false, "ok %d %d %d", 123, 456, 789);
    }, "获取整数组"); // 补充描述

    registry.registerCommand('#', "CMDMODE", [](const std::string& cmdBody, StreamSink& response) {
        uint32_t mode;
        // 解析"CMDMODE 123"格式
        if (sscanf(cmdBody.c_str(), "CMDMODE %lu", &mode) == 1) {
            Respond(response, false, "Set command mode to [%lu]", mode);
        } else {
            Respond(response, true, "Invalid CMDMODE param: %s", cmdBody.c_str());
        }
    }, "设置命令模式"); // 补充描述

    // 4. 注册前缀'$'的命令：ISR_STACK/TEST_PSRAM
    registry.registerCommand('$', "ISR_STACK", [](const std::string&, StreamSink& response) {
        RespondIsrStackUsageInWords(response);
    }, "查看ISR栈使用"); // 补充描述

    registry.registerCommand('$', "TEST_PSRAM", [](const std::string&, StreamSink& response) {
        Respond(response, false, "PSRAM Test started");
        // PSRAM_Test(OCTOSPI1_BASE); // 你测试函数
    }, "测试PSRAM"); // 补充描述
    registry.registerCommand('$', "TEST_SD_SPEED", [](const std::string&, StreamSink& response) {
        Respond(response, false, "SD Speed Test started");
    	fxSdTestSpeed();	/* SD卡速度测试 */
    }, "测试SD卡速度"); // 补充描述




}

// 新的OnAsciiCmd：彻底抛弃if-else，调用注册器解析
void OnAsciiCmd(const char* _cmd, size_t _len, StreamSink _responseChannel) {
    CommandRegistry::getInstance().parseAndExecute(_cmd, _len, _responseChannel);
}
