#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <map>
#include <functional>

#include "ConfigUpdater.h"
#include "document.h"
#include "ostreamwrapper.h"
#include "prettywriter.h"
#include "istreamwrapper.h"

bool ConfigUpdater::UpdateUseProgressBar(const std::string &filename, bool newValue)
{
    // 读取现有配置文件
    std::ifstream ifs(filename);
    if (!ifs.is_open())
    {
        std::cerr << "无法打开文件进行读取。\n";
        return false;
    }

    // 使用RapidJSON解析JSON文件
    rapidjson::Document document;
    rapidjson::IStreamWrapper isw(ifs);
    if (document.ParseStream(isw).HasParseError())
    {
        std::cerr << "解析JSON文件出错。\n";
        return false;
    }

    ifs.close(); // 读取完成，关闭输入流

    // 检查 "skipListBenchmark" 和 "useProgressBar" 成员是否存在
    if (!document.HasMember("skipListBenchmark") || !document["skipListBenchmark"].IsObject() || 
        !document["skipListBenchmark"].HasMember("useProgressBar") || !document["skipListBenchmark"]["useProgressBar"].IsBool())
    {
        std::cerr << "JSON结构无效。\n";
        return false;
    }

    // 更新 "useProgressBar" 的值
    document["skipListBenchmark"]["useProgressBar"].SetBool(newValue);

    // 将更新后的JSON写回文件
    std::ofstream ofs(filename);
    if (!ofs.is_open())
    {
        std::cerr << "无法打开文件进行写入。\n";
        return false;
    }

    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
    document.Accept(writer);

    ofs.close(); // 写入完成，关闭输出流
    return true;
}

bool ConfigUpdater::UpdateUseRandRNG(const std::string &filename, bool newValue)
{
    // 打开并读取配置文件
    std::ifstream ifs(filename);
    if (!ifs.is_open()) return false;
    
    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(ifs);
    if (doc.ParseStream(isw).HasParseError()) return false;
    
    // 确认 skipListBenchmark 字段存在
    if (!doc.HasMember("skipListBenchmark") || !doc["skipListBenchmark"].IsObject()) return false;
    
    auto& skipListBenchmark = doc["skipListBenchmark"];
    
    // 更新 useRandRNG 字段
    if (!skipListBenchmark.HasMember("useRandRNG") || !skipListBenchmark["useRandRNG"].IsBool()) return false;
    skipListBenchmark["useRandRNG"].SetBool(newValue);
    
    // 写回配置文件
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return false;
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    ofs << buffer.GetString();
    return true;
}

bool promptForBoolean(const std::string& prompt)
{
    while (true)
    {
        std::cout << prompt << " (1: 是, 0: 否): ";
        int choice;
        std::cin >> choice;
        if (std::cin.fail() || (choice != 0 && choice != 1))
        {
            std::cin.clear();   // 清除错误标志
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效。请输入1或0。\n";
        }
        else
        {
            return choice == 1;    // 返回布尔值
        }
    }
}

bool updateConfigFile(const std::string& configFilePath, const std::string& section, const std::string& field, bool value)
{
    // 读取配置文件
    std::ifstream ifs(configFilePath);
    if (!ifs.is_open())
    {
        std::cerr << "无法打开配置文件进行读取: " << configFilePath << std::endl;
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);
    ifs.close();

    // 检查文档是否正确解析，以及是否存在指定部分
    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember(section.c_str()))
    {
        std::cerr << "配置文件解析错误或缺少指定部分: " << section << std::endl;
        return false;
    }

    // 更新字段值
    rapidjson::Value& targetSection = doc[section.c_str()];
    if (!targetSection.IsObject())
    {
        std::cerr << "配置部分 " << section << " 不是一个对象。\n";
        return false;
    }

    targetSection[field.c_str()].SetBool(value);

    // 写入配置文件
    std::ofstream ofs(configFilePath);
    if (!ofs.is_open())
    {
        std::cerr << "无法打开配置文件进行写入: " << configFilePath << std::endl;
        return false;
    }

    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    doc.Accept(writer);
    ofs.close();

    return true;
}

void updateConfiguration()
{
    std::string configFilePath = "C:/SoftWare/VScode-dir/KVengine_cpp/config.json";
    
    // 定义配置字段及其对应更新函数的映射
    std::map<std::string, std::function<void()>> configOptions = {
        {"useProgressBar", [&](){
            bool newValue = promptForBoolean("是否开启进度条功能");
            if (updateConfigFile(configFilePath, "skipListBenchmark", "useProgressBar", newValue)) {
                std::cout << "配置文件已更新。\n";
            } else {
                std::cerr << "配置文件更新失败。\n";
            }
        }},
        {"useRandRNG", [&](){
            bool newValue = promptForBoolean("是否使用标准rand()作为随机数生成器");
            if (updateConfigFile(configFilePath, "skipListBenchmark", "useRandRNG", newValue))
            {
                std::cout << "配置文件已更新。\n";
            }
            else
            {
                std::cerr << "配置文件更新失败。\n";
            }
        }}
    };

    // 提示用户选择要更新的配置字段
    std::cout << "请选择要更新的配置字段:\n";
    int optionIndex = 1;
    for (const auto& option : configOptions)
    {
        std::cout << optionIndex++ << ": " << option.first << '\n';
    }
    std::cout << "请输入选项编号: ";
    
    int selection;
    std::cin >> selection;
    if (std::cin.fail() || selection < 1 || selection > configOptions.size())
    {
        std::cin.clear();   // 清除错误标志
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "输入无效。请重新输入。\n";
    }
    else
    {
        // 查找并执行相应的更新函数
        auto it = std::next(configOptions.begin(), selection - 1);
        it->second();
    }
}