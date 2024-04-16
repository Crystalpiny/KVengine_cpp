#ifndef CONFIGUPDATER_H
#define CONFIGUPDATER_H

#include <string>

/**
 * @class ConfigUpdater
 * @brief 提供修改配置文件的功能。
 *
 * ConfigUpdater 类包含静态成员函数，用于更新 JSON 配置文件中的特定值。
 */
class ConfigUpdater
{
public:
    /**
     * @brief 静态函数，修改配置文件中的 useProgressBar 值。
     *
     * @param filename 配置文件的路径和名称。
     * @param newValue 新的 useProgressBar 值（true 或 false）。
     * @return bool 返回更新操作是否成功。成功返回 true，失败返回 false。
     *
     * @details
     * 调用此函数会打开指定的 JSON 配置文件，解析文件内容，并更新 useProgressBar 字段的值。
     * 如果在打开文件、解析 JSON 或写入更新时遇到任何错误，则函数返回 false。
     * 如果更新成功，则将修改后的 JSON 写回到文件，并返回 true。
     *
     * @note
     * - 函数使用了 RapidJSON 库来解析和修改 JSON 文件。
     * - 函数在更新结束后确保已经关闭所有打开的文件流。
     * - 如果配置文件的结构不包含预期的字段或字段类型不正确，函数将报错并返回 false。
     */
    static bool UpdateUseProgressBar(const std::string &filename, bool newValue);

    /**
     * @brief 静态函数，修改配置文件中的 useRandRNG 值。
     *
     * @param filename 配置文件的路径和名称。
     * @param newValue 新的 useRandRNG 值（true 或 false）。
     * @return bool 返回更新操作是否成功。成功返回 true，失败返回 false。
     * 
     * @details
     * 调用此函数会打开指定的 JSON 配置文件，解析文件内容，并更新 useRandRNG 字段的值。
     * 如果配置文件无法读取或字段类型不正确，函数将返回 false。
     * 成功更新后，将修改后的 JSON 写回到文件，并返回 true。
     * 
     * @note
     * - 函数使用 RapidJSON 库来解析和修改 JSON 文件。
     * - 函数在更新结束后确保已经关闭所有打开的文件流。
     */
    static bool UpdateUseRandRNG(const std::string &filename, bool newValue);
};

bool promptForBoolean(const std::string &prompt);
bool updateConfigFile(const std::string &configFilePath, const std::string &section, const std::string &field, bool value);
void updateConfiguration();
#endif // CONFIGUPDATER_H