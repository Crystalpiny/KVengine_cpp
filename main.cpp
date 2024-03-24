#include <iostream>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"

/**
 * @brief 主函数
 *        这个程序展示了一个菜单，允许用户选择执行不同的功能，直到选择退出程序。
 *        1. 进行Benchmark测试
 *        2. 其他功能
 *        3. 退出程序
 *        用户输入非法时，程序会提示重新输入。
 * 
 * @return int 返回0表示程序正常退出。
 */
int main()
{
    system("chcp 65001");   // 设置字符编码为UTF-8，解决中文乱码问题。

    bool skiplist_benchmark_executed = false; // 标志变量，用于标记Benchmark测试是否已经执行过。

    while (true)
    {
        std::cout << "选择操作：\n1. 进行Benchmark测试\n2. 跳表API接口测试\n3. 其他功能\n4. 退出程序\n请输入选项:" << std::endl;
        int choice;
        std::cin >> choice;

        if (std::cin.fail()) {
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
                    skiplist_benchmark_executed = true; // 标记Benchmark测试已经执行过。
                }
                break;
            case 2:
                // 进行跳表API接口测试。
                skiplist_usual_use();
                break;
            case 3:
                // 进入其他功能的处理。
                break;
            case 4:
                std::cout << "退出程序。" << std::endl;
                return 0;
            default:
                std::cout << "无效选项。请重新输入。\n";
        }
    }
    return 0;
}