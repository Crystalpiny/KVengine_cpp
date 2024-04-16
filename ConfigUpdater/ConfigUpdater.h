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

/**
 * @brief 提示用户做出是或否的选择。
 *
 * @param prompt 显示给用户的提示字符串。
 * @return 用户输入的布尔选择。
 * @details 这个函数不断地提示用户输入，直到得到有效的输入（1或0）为止。
 *          如果用户输入无效，它会清除输入流并提示用户重新输入。
 * @note 输入被限制为整数1或0，其中1代表是，0代表否。
 */
bool promptForBoolean(const std::string &prompt);

/**
 * @brief 更新配置文件中的特定字段。
 *
 * @param configFilePath 配置文件的路径。
 * @param section 配置文件中要更新的部分。
 * @param field 要更新的字段。
 * @param value 要设置的新布尔值。
 * @return 更新成功返回true，否则返回false。
 * @details 函数首先尝试读取配置文件，并解析为JSON对象。
 *          然后，它会检查指定的部分并更新字段的值。
 *          最后，它会将更新后的JSON对象写回配置文件。
 * @note 使用了RapidJSON库来处理JSON数据。
 *       需要确保配置文件的路径是正确的，并且文件具有正确的读写权限。
 */
bool updateConfigFile(const std::string &configFilePath, const std::string &section, const std::string &field, bool value);

/**
 * @brief 主函数，处理更新配置的逻辑。
 *
 * @details 该函数提示用户选择要更新的配置字段。
 *          根据用户的选择，执行相应的更新逻辑。
 * @note 该函数依赖于提供的配置选项映射。
 *       映射中每个选项都有一个对应的lambda函数，用于处理用户的选择。
 */
void updateConfiguration();
#endif // CONFIGUPDATER_H