#ifndef JSON_TEST_H
#define JSON_TEST_H

#include <string>

/**
 * @brief 获取指定文件夹下最新的JSON文件名。
 *
 * 此方法查询给定文件夹，搜索所有扩展名为.json的文件，
 * 并返回最新（根据修改时间）的文件的完整路径名。
 *
 * @param folder_path 指定文件夹的路径。
 * @return 返回最新.json文件的完整路径名。如果无法找到.json文件，则返回空字符串。
 */
std::string get_latest_file(const std::string &folder_path);

/**
 * @brief 测试跳表的数据保存和加载接口。
 *
 * @details
 * 此函数先创建一个跳表实例，并插入一些示例元素。然后，
 * 将跳表的内容保存到一个JSON文件中。之后，从该JSON文件
 * 加载数据到一个新的跳表实例，并验证两个跳表是否相等。
 */
void test_load_save_interface();

#endif // JSON_TEST_H