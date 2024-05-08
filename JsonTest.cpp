#include <filesystem>
#include <iostream>

#include "JsonTest.h"
#include "skiplist.h"

std::string get_latest_file(const std::string& folder_path)
{
  std::filesystem::path latest_file;
  auto latest_time = std::filesystem::file_time_type::min();

  for (const auto& entry : std::filesystem::directory_iterator(folder_path))
  {
    if (entry.is_regular_file() && entry.path().extension() == ".json")
    {
      auto current_time = std::filesystem::last_write_time(entry);
      if (current_time > latest_time)
      {
        latest_time = current_time;
        latest_file = entry.path();
      }
    }
  }

  return latest_file.string();
}

void test_load_save_interface()
{
    // 创建并初始化一个 SkipList 实例
    SkipList<int, std::string> originalList(10);
    originalList.insert_element(1, "Value 1");
    originalList.insert_element(2, "Value 2");
    originalList.insert_element(3, "Value 3");
    originalList.display_list();
    
    // 将原始跳表保存到JSON文件
    std::string basic_file_name = "test_skiplist";
    originalList.save_to_json(basic_file_name);
    std::cout << "Saved original skiplist to JSON file successfully.\n";

    // 从JSON文件加载跳表内容到一个新的实例
    SkipList<int, std::string> newList(10);
    std::string store_dir = "C:/SoftWare/VScode-dir/KVengine_cpp/store";
    std::string latest_file = get_latest_file(store_dir);
    newList.load_from_json(latest_file);
    newList.display_list();
    std::cout << "Loaded skiplist from JSON file successfully.\n";

    // 验证新加载的跳表是否与原始跳表相同
    if (newList.skiplist_equals(originalList))
    {
        std::cout << "Test PASSED: The original and loaded skip lists are equal.\n";
    }
    else
    {
        std::cout << "Test FAILED: The original and loaded skip lists are not equal.\n";
    }
}