#include <iostream>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"
#include "ConfigUpdater/ConfigUpdater.h"

/**
 * @file main.cpp
 * @brief 主程序入口和用户界面交互实现。
 *
 * 提供一个用户菜单来选择不同的操作，包括执行基准测试、API测试、修改配置文件及退出程序。
 *
 * @remark 使用 system("chcp 65001") 来设置控制台输出为UTF-8，以解决中文乱码问题。
 *
 * @return int 程序退出时返回的状态码。正常退出时返回0。
 *
 * @details
 * 程序运行后，用户将会看到一个包含多个选项的菜单：
 * - 1: 执行Benchmark基准测试，如果已经执行过，将提示用户测试已完成。
 * - 2: 进行跳表API接口测试。
 * - 3: 进入命令识别模式。
 * - 4: 修改配置文件中的进度条显示选项。
 * - 5: 退出程序。
 * 用户需要输入对应的数字来选择想要执行的操作。如果输入无效，程序将提示重新输入。
 *
 * @note
 * - 使用 bool 变量 `skiplist_benchmark_executed` 来标记是否已经执行过基准测试。
 * - 用户输入的处理包括错误检测和处理，保证即使输入无效程序也不会异常退出。
 * - 修改配置文件的操作将调用 ConfigUpdater 类的静态成员函数 UpdateUseProgressBar。
 */
int main()
{
    system("chcp 65001");   // 设置字符编码为UTF-8，解决中文乱码问题。

    bool skiplist_benchmark_executed = false; // 标志变量，用于标记Benchmark测试是否已经执行过。

    while (true)
    {
        std::cout << "选择操作：\n1. 进行Benchmark测试\n2. 跳表API接口测试\n3. 命令识别模式\n4. 修改配置文件\n5. 退出程序\n请输入选项:" << std::endl;
        int choice;
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效。请输入数字。\n";
            continue;
        }

        switch (choice)
        {
            case 1:
                if (skiplist_benchmark_executed)
                {
                    // 如果Benchmark测试已经执行过，提示用户并返回主菜单。
                    std::cout << "Benchmark测试已经执行过。请选择其他操作。\n";
                }
                else
                {
                    // 进入Benchmark测试框架，并标记Benchmark测试已执行。
                    skiplist_benchmark();
                    skiplist_benchmark_executed = true;
                }
                break;
            case 2:
                // 进行跳表API接口测试。
                skiplist_usual_use();
                break;
            case 3:
                // 进入命令识别模式。
                {
                    // 创建一个整型键和字符串值的跳表，最大层级为 10
                    SkipList<int, std::string> skipList(10);
                    // 将跳表实例传递给控制台接口
                    SkipListConsole<int, std::string> console(skipList);
                    // 运行控制台
                    console.run();
                }
                break;
            case 4:
                // 修改配置文件
                updateConfiguration();
                break;
            case 5:
                std::cout << "退出程序。" << std::endl;
                return 0;
            default:
                std::cout << "无效选项。请重新输入。\n";
        }
    }
    return 0;
}