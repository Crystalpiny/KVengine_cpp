#include "ConfigUpdater.h"
#include <fstream>
#include <iostream>
#include <document.h>
#include <ostreamwrapper.h>
#include <prettywriter.h>
#include <istreamwrapper.h>

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