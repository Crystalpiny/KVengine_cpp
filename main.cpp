#include <iostream>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"

int main()
{

    system("chcp 65001");   // 解决编码格式致使中文乱码问题

    std::cout << "选择操作：\n1. 进行Benchmark测试\n2. 其他功能\n请输入选项(1或2):" << std::endl;
    int choice;
    std::cin >> choice;
    switch (choice)
    {
    case 1:
        // 进入benchmark测试框架
        skiplist_benchmark();
        // skiplist_usual_use();        // 函数接口效果测试.
        break;
    case 2:
        // 进入命令识别模式
        break;
    default:
        std::cout << "无效选项。" << std::endl;
    }
    
    return 0;
}

// int main()
// {
//     insert_test(); // 随机写入测试，计算QPS
//     // completedTasks = 0;   //重置计数器
//     search_test(); // 随机读取测试，计算QPS
//     // skiplist_usual_use();          //函数接口使用
//     return 0;
// }