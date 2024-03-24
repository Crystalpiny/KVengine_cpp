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

    while (true)  // 使用无限循环来保持程序运行，直到用户选择退出。
    {
        std::cout << "选择操作：\n1. 进行Benchmark测试\n2. 跳表API接口测试\n3. 其他功能\n4. 退出程序\n请输入选项:" << std::endl;
        int choice;  // 存储用户的选择。
        std::cin >> choice;  // 从标准输入读取用户的选择。

        if (std::cin.fail()) {  // 检查输入是否失败，例如由于输入非数字。
            std::cin.clear();  // 清除输入流的错误标志。
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // 忽略错误的输入直到下一个换行符。
            std::cout << "输入无效。请输入数字。\n";  // 提示用户输入有效的数字。
            continue;  // 跳过循环的剩余部分，重新开始循环。
        }

        switch (choice)  // 根据用户的选择执行相应的操作。
        {
            case 1:
                // 进入Benchmark测试框架。
                skiplist_benchmark();
                break;
            case 2:
                // 进行跳表API接口测试。
                skiplist_usual_use();
                break;
            case 3:
                // 进入其他功能的处理。
                break;
            case 4:
                std::cout << "退出程序。" << std::endl;  // 打印退出提示。
                return 0;  // 返回0，退出程序。
            default:
                std::cout << "无效选项。请重新输入。" << std::endl;  // 提示用户重新输入有效选项。
        }
    }
    
    // 实际上，这里的 return 0 是多余的，因为程序会在 case 3 中通过 return 0 退出。
    return 0;
}