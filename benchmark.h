#ifndef BENCHMARK_H
#define BENCHMARK_H

#define MULTI_NUM_FOR_INPUT (1000000)   //  用户输入数据量的乘数,简化用户操作

void init_benchmark_data();

void insertElement(int tid);

void getElement(int tid);

void insert_test();

void search_test();

void usual_use();

#endif // BENCHMARK_H