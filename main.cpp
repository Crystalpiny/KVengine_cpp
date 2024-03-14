#include <iostream>

#include "skiplist.h"
#include "ThreadPool.h"
#include "benchmark.h"

int main() {
    system("chcp 65001");
    std::cout << "选择操作：\n1. 进行Benchmark测试\n2. 其他功能\n请输入选项(1或2):" << std::endl;
    int choice;
    std::cin >> choice;
    switch (choice)
    {
    case 1:
        insert_test();      // 进行插入测试,测试QPS.
        search_test();      // 进行搜索测试,测试QPS.
        usual_use();        // 函数接口效果测试.
        break;
    case 2:
        // 调用其他功能
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
//     // usual_use();          //函数接口使用
//     return 0;
// }