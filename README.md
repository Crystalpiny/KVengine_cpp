# KV存储引擎

## 项目介绍

基于跳表实现的K-V(键-值)存储引擎，使用C++实现。

测试PC机 CPU 8C16T.

根据测试数据：本程序在平均情况下的随机读写QPS如下：

- 随机写入QPS：150w 左右
- 随机读取QPS：2700w 左右

引入单任务队列线程池管理线程资源，提高程序效能。
使用了良好的编码规范、注释，帮助阅读者理解代码。

跳表类提供函数接口：插入元素、删除元素、查询元素、跳表显示、数据持久化、从文件加载数据、跳表大小显示。

线程池类提供函数接口：构造函数初始化、提交任务到任务队列、析构函数回收资源

PS：线程池的实现是我在Jakob Progsch, Václav Zeman实现的线程池基础上稍作改动得来的。

原ThreadPool源码地址：https://github.com/progschj/ThreadPool

### 项目中文件

- skipList.h	  跳表类核心实现
- ThreadPool.h   线程池核心实现
- main.cpp          包含使用跳表进行元素操作、随机写入测试、随机读取测试
- README.md     项目说明文档
- CMakeList.txt   cmake配置文件
- store                  数据持久化文件路径
- COPYINGofThreadPool    ThreadPool使用协议

### skipList函数接口

- insert_element	插入元素

- delete_element	删除元素

- search_element	查询元素

- display_list		显示跳表

- dump_file		数据持久化到磁盘

- load_file		    从文件加载数据

- size			    显示跳表元素数量

### ThreadPool函数接口

- ThreadPool(size_t)	根据参数初始化线程池

- enqueue(F&& f, Args&&... args)	将任务添加到任务队列

### main函数接口

- insert_test	随机写入测试，获取QPS

- search_test	随机读取测试，获取QPS

- usual_use	   skiplist函数接口使用

## 存储引擎性能量化

跳表最大层级：18

线程数：16

每种数据量下测试数据取10次平均值

### 随机写入QPS

| 写入数据规模(万条) | 耗时(秒) | QPS      |
| :----------------- | :------- | :------- |
| 100                | 0.63689  | 157.013w |
| 300                | 1.96929  | 152.339w |
| 500                | 3.35483  | 148.039w |
| 1000               | 6.75545  | 148.029w |



### 随机读取QPS

| 读取数据规模(万条) | 耗时(秒) | QPS      |
| ------------------ | -------- | -------- |
| 100                | 0.034729 | 2879.44w |
| 300                | 0.109158 | 2748.3w  |
| 500                | 0.197758 | 2528.34w |
| 1000               | 0.412144 | 2426.34w |

## 项目运行方式

### clion运行

本项目是在windows平台下的clion中直接编写，如果你也同样是使用Clion、vs、vscode类似的IDE，可以直接运行或将.h .cpp 文件内容复制后运行。

### cmake运行

1. 确保你已经安装了 CMake，并且将其添加到了系统的环境变量中。
2. 创建一个新的文件夹，作为你的构建目录，例如 `build`。
3. 在构建目录中打开终端或命令提示符。
4. 将你的 `CMakeLists.txt` 文件复制到构建目录中。
5. 运行以下命令，生成构建文件：`cmake ..`    这会告诉 CMake 在上一级目录中查找 `CMakeLists.txt` 文件，并生成相应的构建文件
6. 根据你的操作系统和构建文件类型，使用适当的构建工具来编译你的项目。例如，如果你的构建文件是 Makefile，可以运行以下命令来进行编译：`make`
7. 等待编译完成后，在构建目录中会生成可执行文件，可以通过以下命令来运行它：`./KVengine`

## 项目使用说明

main.cpp的主函数main中目前内容如下：

```c++
int main() {
    insert_test();  //  随机写入测试，计算QPS
    //completedTasks = 0;  // 重置计数器
    search_test();  //  随机读取测试，计算QPS
    //usual_use();    //  函数接口使用
    return 0;
}
```

search_test()函数不可单独使用，那样是在空的跳表中检索数据，QPS会非常高，但那不是真的。

常规的创建线程和回收线程的方法以及使用线程池的方法代码中都有实现，你可以通过注释、取消注释切换线程资源管理的方式。

skiplist中实现的函数接口中有日志信息输出，不过那部分代码暂时被注释掉了。如果你想查看这些信息，请你找到相应函数的位置，取消注释就可以看到日志信息输出。