#include <csignal>
#include <iostream>
#include <sstream>
#include "Util/logger.h"
#include "Network/TcpClient.h"
#include <thread>

using namespace std;
using namespace toolkit;

class RedisClient : public TcpClient {
public:
    using Ptr = std::shared_ptr<RedisClient>;
    RedisClient() : TcpClient() {
        DebugL << "RedisClient created.";
    }

    ~RedisClient() {
        DebugL << "RedisClient destroyed.";
    }

protected:
    // 连接成功后的处理
    virtual void onConnect(const SockException &ex) override {
        if (ex) {
            ErrorL << "连接失败: " << ex.what();
        } else {
            InfoL << "成功连接到Redis服务器!" << endl;
            sendUserCommand();  // 启动命令发送
        }
    }

    // 收到数据的处理
    virtual void onRecv(const Buffer::Ptr &pBuf) override {
        string response = pBuf->toString();
        std::cout << "从Redis接收到的数据: " << response << std::endl;
        DebugL << "从Redis接收到的数据: " << response;

        // 解析响应
        parseRedisResponse(response);
    }

    // 处理连接错误事件
    virtual void onError(const SockException &ex) override {
        WarnL << ex.what();
    }

    // 定时任务，每5秒发送一个 INFO 命令
    virtual void onManager() override {
    }

private:
    int _nTick = 0;

    // 处理用户输入
    void sendUserCommand() {
        // 启动线程处理用户输入
        std::thread([this]() {
            string command;
            while (true) {
                cout << "请输入Redis命令（输入'exit'退出）: " << endl;
                getline(cin, command);

                if (command == "exit") {
                    break;
                }

                string redisCommand = formatRedisCommand(command);
                if (!redisCommand.empty()) {
                    sendCommand(redisCommand); // 发送格式化的命令
                } else {
                    cerr << "无效的命令格式。" << endl;
                }
            }
            this->shutdown();  // 结束连接
        }).detach(); // 以独立线程的形式处理用户输入
    }

    // 格式化Redis协议命令（RESP协议）
    string formatRedisCommand(const string &command) {
        stringstream ss;
        vector<string> tokens;
        string token;

        // 将输入按空格分割成多个参数
        stringstream inputStream(command);
        while (inputStream >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            return "";  // 空命令无效
        }

        // 构造Redis协议格式
        ss << "*" << tokens.size() << "\r\n";  // 命令包含的参数数量
        for (const auto &arg : tokens) {
            ss << "$" << arg.size() << "\r\n" << arg << "\r\n";
        }

        return ss.str();
    }

    // 发送Redis命令
    void sendCommand(const string &command) {
        send(command);  // 通过TCP发送数据
        DebugL << "发送命令: " << command;
    }

    // 解析Redis响应
    void parseRedisResponse(const string &response) {
        if (response.empty()) {
            cerr << "接收到的响应为空。" << endl;
            return;
        }

        // 基本的RESP协议处理
        if (response[0] == '+') {
            // 简单字符串响应, 如 "+OK"
            cout << "响应: " << response.substr(1) << endl;
        } else if (response[0] == '-') {
            // 错误响应, 如 "-ERR unknown command"
            cerr << "错误: " << response.substr(1) << endl;
        } else if (response[0] == ':') {
            // 整数响应, 如 ":1"
            cout << "整数: " << response.substr(1) << endl;
        } else if (response[0] == '$') {
            // 大块字符串响应, 如 "$6\r\nfoobar\r\n"
            size_t length = std::stoi(response.substr(1, response.find("\r\n") - 1));
            string data = response.substr(response.find("\r\n") + 2, length);
            cout << "大块字符串: " << data << endl;
        } else if (response[0] == '*') {
            // 数组响应, 如 "*2\r\n$3\r\nSET\r\n$3\r\nfoo\r\n"
            size_t numElements = std::stoi(response.substr(1, response.find("\r\n") - 1));
            cout << "数组响应，共有 " << numElements << " 个元素。" << endl;
        } else {
            cerr << "未识别的响应类型。" << endl;
        }
    }
};

int main() {
    RedisClient::Ptr client(new RedisClient());  // 使用智能指针
    client->startConnect("127.0.0.1", 6380);  // 连接服务器

    // 退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });  // 设置退出信号
    sem.wait();
    return 0;
}
