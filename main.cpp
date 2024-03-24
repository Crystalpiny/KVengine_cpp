#include <iostream>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"

/**
 * @brief 主函数
 * 
 * 程序的入口函数，提供用户选择操作的菜单，并执行相应的功能。
 * 
 * @return int 返回程序的退出状态码
 * 
 * @note 
 * - 该函数依赖于 iostream 库中的 std::cout、std::cin 对象，以及 system 函数和 std::numeric_limits<std::streamsize>::max()。
 * - 使用 bool 变量 `skiplist_benchmark_executed` 标记 Benchmark 测试是否已经执行过。
 * - 进入无限循环，直到用户选择退出程序。
 * - 输出操作选择菜单，并接收用户输入的选项。
 * - 检查用户输入是否为数字，如果不是，清空输入流并提示重新输入。
 * - 使用 switch 语句根据用户选择执行相应的操作。
 * - 如果选择 1，检查 `skiplist_benchmark_executed` 变量，如果已经执行过，提示用户并返回主菜单；否则执行 Benchmark 测试并标记已执行。
 * - 如果选择 2，执行跳表 API 接口测试。
 * - 如果选择 3，进入其他功能的处理。
 * - 如果选择 4，输出退出信息并返回程序退出状态码。
 * - 如果选择其他无效选项，提示用户重新输入。
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
                    skiplist_benchmark_executed = true;
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