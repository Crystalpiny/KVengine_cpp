#ifndef LOG_MOD_H
#define LOG_MOD_H

#include <string.h>

#include <iostream>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include <fstream>
#include <filesystem>
#include <sstream>

const std::string LOG_FOLDER = "C:/SoftWare/VScode-dir/KVengine_cpp/logs"; // 日志文件存放目录
const ssize_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB 最大文件大小
typedef unsigned int thread_id_t;

namespace limlog {

// DigitsTable用于查找100以内的数字。
// 每两个字符对应一位数和十位数。
// 优化数字转换字符串，EXP：58 * 2 = 116,index 116 and 117 is '5' '8'.
static constexpr char DigitsTable[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0',
    '7', '0', '8', '0', '9', '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0', '2', '1', '2',
    '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3',
    '7', '3', '8', '3', '9', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', '5', '0', '5', '1', '5',
    '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6',
    '7', '6', '8', '6', '9', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '8', '0', '8', '1', '8',
    '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9',
    '7', '9', '8', '9', '9'};

/**
 * @brief 对无符号整型数进行格式化，并转存为字符数组
 * 
 * @tparam T 数据类型，需为整型（通过 is_integral 判断）
 * @tparam 默认模板参数为空，根据 T 类型的有效性进行启用或禁用
 * @param v 要格式化的无符号整数
 * @param to 存放结果的字符数组
 * @return size_t 返回字符数组的长度
 *
 * 函数利用循环将整数 v 按两位数的方式转换为对应的字符，存储在字符数组 to 中。
 * 如果 v 最后不足两位数，则单独处理。生成的字符数组的字符顺序与整数 v 相反。
 */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
static inline size_t formatUIntInternal(T v, char to[])
{
  char *p = to;

  while (v >= 100)
  {
    const unsigned idx = (v % 100) << 1;
    v /= 100;
    *p++ = DigitsTable[idx + 1];
    *p++ = DigitsTable[idx];
  }

  if (v < 10)
  {
    *p++ = v + '0';
  }
  else
  {
    const unsigned idx = v << 1;
    *p++ = DigitsTable[idx + 1];
    *p++ = DigitsTable[idx];
  }

  return p - to;
}

/**
 * @brief 对有符号整型数进行格式化，并转存为字符数组。
 * 
 * @tparam T 数据类型，需为整型（通过 `std::is_integral` 判断）
 * @param v 要格式化的有符号整数
 * @param to 存放结果的字符数组
 * @return size_t 返回字符数组的长度
 * 
 * 该函数处理有符号整数的格式化，负数会先转化为正数处理。
 */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
static inline size_t formatSIntInternal(T v, char to[])
{
  char *p = to;

  while (v <= static_cast<T>(-100))
  {
    const T idx = -(v % 100) * 2;
    v /= 100;
    *p++ = DigitsTable[idx + 1];
    *p++ = DigitsTable[idx];
  }

  if (v > static_cast<T>(-10))
  {
    *p++ = -v + '0';
  }
  else
  {
    const T idx = -v * 2;
    *p++ = DigitsTable[idx + 1];
    *p++ = DigitsTable[idx];
  }

  return p - to;
}

/**
 * @brief 格式化整型数（无论正负）为字符串表示。
 * 
 * @tparam T 数据类型，需为整型（通过 `std::is_integral` 检查）
 * @param v 要格式化的整数
 * @param to 存放结果的字符数组
 * @return size_t 返回字符串的长度
 * 
 * 如果整数为负数，会在结果前添加负号。使用辅助函数 `formatSIntInternal` 或 `formatUIntInternal` 来格式化整数。
 */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
static inline size_t formatInt(T v, char *to)
{
  char buf[sizeof(v) * 4];
  size_t signLen = 0;
  size_t intLen = 0;

  if (v < 0)
  {
    *to++ = '-';
    signLen = 1;
    intLen = formatSIntInternal(v, buf);
  }
  else
  {
    intLen = formatUIntInternal(v, buf);
  }

  char *p = buf + intLen;
  for (size_t i = 0; i < intLen; ++i)
    *to++ = *--p;

  return signLen + intLen;
}

/**
 * @brief 格式化无符号整型数，为其指定宽度，不足部分用0填充。
 * 
 * @tparam T 数据类型，需为整型（通过 `std::is_integral` 判断）
 * @param v 要格式化的无符号整数
 * @param to 存放结果的字符数组
 * @param fmtLen 指定的格式化长度
 * @return size_t 返回字符串的实际长度
 * 
 * 如果整数的位数少于指定长度，会在前面填充0以达到指定长度。
 */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
static inline size_t formatUIntWidth(T v, char *to, size_t fmtLen)
{
  char buf[sizeof(v) * 4];
  size_t len = formatUIntInternal(v, buf);
  char *p = buf + len;

  for (size_t i = len; i < fmtLen; i++)
    *to++ = '0';

  size_t minLen = std::min(len, fmtLen);
  for (size_t i = 0; i < minLen; ++i)
    *to++ = *--p;

  return fmtLen;
}

/**
 * @brief 将字符写入字符数组。
 * 
 * @param to 目标字符数组
 * @param c 要写入的字符
 * @return size_t 返回写入的字符数（总是1）
 * 
 * 该函数用于将单个字符写入到指定的数组位置。
 */
static inline size_t formatChar(char *to, char c)
{
  *to = c;
  return sizeof(char);
}

/**
 * 枚举描述日期时间字段的长度。
 */
enum TimeFieldLen : size_t
{
  Year = 4,
  Month = 2,
  Day = 2,
  Hour = 2,
  Minute = 2,
  Second = 2,
};

/**
 * 枚举描述秒的小数部分的长度。
 */
enum SecFracLen : size_t { Sec = 0, Milli = 3, Macro = 6, Nano = 9 };

/**
 * Time 类提供对日期和时间的封装，包括了从标准时间获取到格式化输出的各种操作。
 */
class Time
{
public:
  using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                            std::chrono::nanoseconds>;
  Time() = delete;
  Time(const Time &) = default;
  Time &operator=(const Time &) = default;

  explicit Time(time_t second)
      : Time(TimePoint(std::chrono::seconds(second))) {}
  explicit Time(const TimePoint &tp) : tp_(tp) {}

  // 获取当前时间。
  static Time now() { return Time(std::chrono::system_clock::now()); }

  // 获取年份（4位数，例如1996）。
  int year() const { return toTm().tm_year + 1900; }

  // 获取月份，范围[1, 12]。
  int month() const { return toTm().tm_mon + 1; }

  // 获取月中的日子，范围[1, 28/29/30/31]。
  int day() const { return toTm().tm_mday; }

  // 获取星期几，范围[1, 7]。
  int weekday() const { return toTm().tm_wday; }

  // 获取小时（24小时制），范围[0, 23]。
  int hour() const { return toTm().tm_hour; }

  // 获取小时中的分钟偏移，范围[0, 59]。
  int minute() const { return toTm().tm_min; }

  // 获取分钟中的秒偏移，范围[0, 59]。
  int second() const { return toTm().tm_sec; }

  // 获取秒中的纳秒偏移，范围[0, 999999999]。
  int nanosecond() const { return static_cast<int>(count() % std::nano::den); }

  // 自1970-01-01T00:00:00Z以来经过的纳秒数。
  int64_t count() const { return tp_.time_since_epoch().count(); }

/**
* 获取当前实例所对应的时区名称和UTC偏移秒数。
* 
* 该方法计算当前时间点相对于UTC的偏移量（以秒为单位）和本地时区的名称。
* 偏移量可以是正数、负数或零，表示本地时间相对于UTC的前移或后移量。
* 
* @return 返回一个std::pair对象，它的first元素是一个long int，代表与UTC的偏移秒数；
* second元素是一个字符串，表示时区的名称。
* 如果时区名称和偏移量未被初始化（即第一次调用此函数），它们将被设定并返回。
* 
* 示例返回值：
* 对于位于东八区的时区，可能返回 {28800, "CST"}，
* 其中28800秒代表与UTC的正向偏移8小时，"CST"为该时区的名称。
* 
* 注意：
* - 此方法使用了thread_local关键字使得偏移量和时区名称对每个线程是唯一的，
*   因此在多线程环境中安全使用。
* - 时区名称是基于当前系统环境设置和库函数localtime_r提供的信息导出的，
*   其长度和内容可能会根据不同的系统和时区设置而变化。
* - 时区偏移和名称的初始化仅在方法首次被调用时执行，之后将返回缓存结果，
*   减少对系统调用的重复请求。
*/
  std::pair<long int, std::string> timezone() const
  {
    static thread_local long int t_off = std::numeric_limits<long int>::min();
    static thread_local char t_zone[8];

    if (t_off == std::numeric_limits<long int>::min()) {
      struct tm t;
      time_t c = std::chrono::system_clock::to_time_t(tp_);
      #ifdef __unix__
      localtime_r(&c, &t);
      t_off = t.tm_gmtoff;
      std::copy(t.tm_zone,
                t.tm_zone + std::char_traits<char>::length(t.tm_zone), t_zone);
      #else
      std::strcpy(t_zone, "CST");
      t_off = 28800;
      #endif
    }

    return std::make_pair(t_off, t_zone);
  }

  // 根据RFC3339规范使用标准日期时间格式。
  // 例如：2021-10-10T13:46:58Z 或 2021-10-10T05:46:58+08:00
  std::string format() const { return formatInternal(SecFracLen::Sec); }

  // 使用毫秒的标准日期时间格式。
  std::string formatMilli() const { return formatInternal(SecFracLen::Milli); }

  // 使用微秒的标准日期时间格式。
  std::string formatMacro() const { return formatInternal(SecFracLen::Macro); }

  // 使用纳秒的标准日期时间格式。
  std::string formatNano() const { return formatInternal(SecFracLen::Nano); }

private:
#ifdef _WIN32
    // Windows环境下使用localtime_s
    struct tm toTm() const
    {
        struct tm t;
        time_t c = std::chrono::system_clock::to_time_t(tp_);
        localtime_s(&t, &c); // 适配Windows
        return t;
    }
#else
  /**
  * @brief 将当前时间点转换成 struct tm 格式的时间表示。
  *
  * @return 返回 struct tm 对象，表示本地时间。
  */
  struct tm toTm() const
  {
    struct tm t;
    time_t c = std::chrono::system_clock::to_time_t(tp_);
    localtime_r(&c, &t);    // 使用 localtime_r() 以考虑本地时区

    return t;
  }
#endif

  /**
  * @brief 根据给定的秒数小数长度格式化时间。
  *
  * @param fracLen 小数点后的秒数长度，可以是秒、毫秒、微秒或纳秒。
  * @return 返回一个已格式化的日期时间字符串。
  */
  std::string formatInternal(size_t fracLen) const
  {
    char datetime[40];
    char *p = datetime;
    struct tm t = toTm();

    p += formatDate(p, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    p += formatChar(p, 'T');
    p += formatTime(p, t.tm_hour, t.tm_min, t.tm_sec, fracLen);

    return std::string(datetime, p - datetime);
  }

  /**
  * @brief 格式化日期部分。
  *
  * @param to 指向目标存储区的指针。
  * @param year 年份。
  * @param mon 月份。
  * @param mday 月中的日。
  * @return 返回格式化后的长度。
  */
  size_t formatDate(char *to, int year, int mon, int mday) const
  {
    char *p = to;
    p += formatUIntWidth(year, p, TimeFieldLen::Year);
    p += formatChar(p, '-');
    p += formatUIntWidth(mon, p, TimeFieldLen::Month);
    p += formatChar(p, '-');
    p += formatUIntWidth(mday, p, TimeFieldLen::Day);
    return p - to;
  }

  /**
   * @brief 格式化完整的时间，包括时、分、秒和小数部分。
   *
   * @param to 指向目标存储区的指针。
   * @param hour 小时。
   * @param min 分钟。
   * @param sec 秒。
   * @param fracLen 小数长度。
   * @return 返回格式化后的长度。
   */
  size_t formatTime(char *to, int hour, int min, int sec,
                    size_t fracLen) const {
    char *p = to;
    p += formatPartialTime(p, hour, min, sec, fracLen);
    p += formatTimeOff(p);
    return p - to;
  }

  /**
  * @brief 格式化时间的小时、分钟和秒部分，不包括时间偏移。
  *
  * @param to 指向目标存储区的指针。
  * @param hour 小时。
  * @param min 分钟。
  * @param sec 秒。
  * @param fracLen 小数点后的长度。
  * @return 返回格式化后的长度。
  */
  size_t formatPartialTime(char *to, int hour, int min, int sec,
                           size_t fracLen) const {
    char *p = to;
    p += formatUIntWidth(hour, p, TimeFieldLen::Hour);
    p += formatChar(p, ':');
    p += formatUIntWidth(min, p, TimeFieldLen::Minute);
    p += formatChar(p, ':');
    p += formatUIntWidth(sec, p, TimeFieldLen::Second);
    p += formatSecFrac(p, nanosecond(), fracLen);
    return p - to;
  }

  /**
  * @brief 格式化秒的小数部分。
  *
  * @param to 指向目标存储区的指针。
  * @param frac 秒的小数部分。
  * @param fracLen 小数长度。
  * @return 返回格式化后的长度。
  */
  size_t formatSecFrac(char *to, int frac, size_t fracLen) const
  {
    if (fracLen == 0 || frac == 0)
      return 0;

    char *p = to;
    p += formatChar(p, '.');
    p += formatUIntWidth(frac, p, fracLen);
    return p - to;
  }

  /**
  * @brief 格式化时间偏移部分。
  *
  * @param to 指向目标存储区的指针。
  * @return 返回格式化后的长度。
  */
  size_t formatTimeOff(char *to) const
  {
    long int off = timezone().first;
    char *p = to;

    if (off == 0)
    {
      p += formatChar(p, 'Z');
    }
    else
    {
      p += formatChar(p, off < 0 ? '-' : '+');
      p += formatUIntWidth(off / 3600, p, TimeFieldLen::Hour);
      p += formatChar(p, ':');
      p += formatUIntWidth(off % 3600, p, TimeFieldLen::Minute);
    }

    return p - to;
  }

  TimePoint tp_;
};

/**
 * @brief 获取当前线程的ID。
 *
 * 这个函数使用 thread_local 关键字保存每个线程独立的线程ID，确保只在每个线程中只进行一次计算。
 *
 * @return 返回当前线程的ID，如果线程ID未被初始化（在本线程中首次调用此函数），则生成后返回。
 *
 * 注意：
 * - 这个函数是线程安全的，因为它为每个线程返回一个独立的线程ID。
 * - 线程ID在首次调用时计算并保存，在之后的调用中直接返回保存的线程ID，从而避免不必要的调用。
 */
inline thread_id_t gettid()
{
  static thread_local thread_id_t t_tid = 0;

  if (t_tid == 0)
  {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    ss >> t_tid;
  }
  return t_tid;
}

enum LogLevel : uint8_t { kTrace, kDebug, kInfo, kWarn, kError, kFatal };

/**
 * @brief 将日志等级枚举转换为长度为5的字符串表示。
 *
 * 该函数提供一种将日志等级（如：TRACE, DEBUG, INFO等）从枚举转换为字符串表示的方便方法，
 * 便于在日志输出等场景中使用。
 *
 * @param level 日志等级的枚举值。有效值包括：
 *              - TRACE (对应 "TRAC")
 *              - DEBUG (对应 "DEBU")
 *              - INFO  (对应 "INFO")
 *              - WARN  (对应 "WARN")
 *              - ERROR (对应 "ERRO")
 *              - FATAL (对应 "FATA")
 *              
 * @return 返回一个指向字符数组的指针，该数组以 null 终止，表示输入等级对应的字符串表示。
 *         如果传入的等级值超出了预期范围，程序的行为是未定义的。
 *
 * 注意：
 * - 调用者不应修改返回的字符串。
 * - 该函数依赖于硬编码的字符串数组索引，需要确保传入的LogLevel枚举值与字符串数组中的位置匹配。
 * - 由于使用了静态字符串数组，该方法是线程安全的。 
 */
static const char *stringifyLogLevel(LogLevel level)
{
  const char *levelName[] = {"TRAC", "DEBU", "INFO", "WARN", "ERRO", "FATA"};
  return levelName[level];
}

// 循环FIFO阻塞生产/消费字节队列。保存日志信息以等待后台线程消费。每个线程都存在一个此队列。
class BlockingBuffer
{
public:
  BlockingBuffer() : producePos_(0), consumePos_(0), consumablePos_(0) {}

  // 缓冲区大小。
  uint32_t size() const { return kBlockingBufferSize; }

  // 已使用的字节数。
  // 可能被不同的线程调用，因此添加内存屏障以确保读取最新的*Pos_值。
  uint32_t used() const
  {
    std::atomic_thread_fence(std::memory_order_acquire);
    return producePos_ - consumePos_;
  }

  // 未使用的字节数。
  uint32_t unused() const { return kBlockingBufferSize - used(); }

  // 重置缓冲区位置。
  void reset()
  {
    producePos_ = 0;
    consumePos_ = 0;
    consumablePos_ = 0;
  }

  // 上一个完整日志结束的位置。
  uint32_t consumable() const {
    std::atomic_thread_fence(std::memory_order_acquire);
    return consumablePos_ - consumePos_;
  }

  // 增加可消费位置 n 个字节完整日志的长度。
  void incConsumablePos(uint32_t n) {
    consumablePos_ += n;
    std::atomic_thread_fence(std::memory_order_release);
  }

  // 指向消费位置的指针。
  char *data() { return &storage_[offsetOfPos(consumePos_)]; }

  // 消费 n 字节数据并仅移动消费位置。
  void consume(uint32_t n) { consumePos_ += n; }

  // 将 n 字节数据消费到 to 之中。
  uint32_t consume(char *to, uint32_t n)
  {
    // 可消费的字节数。
    uint32_t avail = std::min(consumable(), n);

    // 消费位置到缓冲区末尾的偏移量。
    uint32_t off2End = std::min(avail, size() - offsetOfPos(consumePos_));

    // 首先将数据从消费位置开始直到缓冲区末尾的数据放入。
    memcpy(to, storage_ + offsetOfPos(consumePos_), off2End);

    // 然后将剩余的数据开始放入缓冲区的开头。
    memcpy(to + off2End, storage_, avail - off2End);

    consumePos_ += avail;
    std::atomic_thread_fence(std::memory_order_release);

    return avail;
  }

  /**
 * @brief 从给定的源数据复制 n 字节到阻塞队列（生产操作）。
 *
 * @details 具体来说，首先计算当前缓冲区可用空间和源数据大小的较小值，
 *    然后再根据可用空间计算出两个偏移量：一个是从当前生产位置到缓冲区末尾的距离，
 *    另一个是剩余的到达缓冲区起始位置的距离。之后为了保持循环队列的特性，
 *    首先将源数据的起始部分复制到当前生产位置至缓冲区末尾的空间中，
 *    然后如果还有剩余的数据，将其复制到缓冲区的前部直到数据复制完成。
 *
 *    在复制完成后，生产位置会向后移动n个位置。然后设置内存屏障，防止编译器和CPU乱序执行。
 *
 * @note 本方法是线程安全的。如果缓冲区的空闲空间不足以容纳需要复制的数据，
 *   此方法会阻塞直到有足够的空间为止。因此，此方法可能会阻塞调用线程。
 *   若要避免阻塞，调用者应确保此方法在有足够的空闲空间时调用。
 *
 * @param from 指向源数据的指针。
 * @param n 源数据的大小（字节数）。
 */
  void produce(const char *from, uint32_t n)
  {
    n = std::min(size(), n);
    while (unused() < n)
      /* blocking */;

    // 生产位置到缓冲区末尾的偏移量。
    uint32_t off2End = std::min(n, size() - offsetOfPos(producePos_));

    // 首先将数据从生产位置开始直到缓冲区末尾的数据放入。
    memcpy(storage_ + offsetOfPos(producePos_), from, off2End);

    // 然后将剩余的数据开始放入缓冲区的开头。
    memcpy(storage_, from + off2End, n - off2End);

    producePos_ += n;
    std::atomic_thread_fence(std::memory_order_release);
  }

private:
  // 从缓冲区开始计算的位置偏移量。
  uint32_t offsetOfPos(uint32_t pos) const { return pos & (size() - 1); }

  static const uint32_t kBlockingBufferSize = 1024 * 1024 * 1; // 1 MB
  uint32_t producePos_;
  uint32_t consumePos_;
  uint32_t consumablePos_; // 每次增加一个完整日志长度。
  char storage_[kBlockingBufferSize]; // 缓冲区大小为2的幂次方。
};

using OutputFunc = ssize_t (*)(const char *, size_t);

struct StdoutWriter
{
  static ssize_t write(const char *data, size_t n)
  {
    return fwrite(data, sizeof(char), n, stdout);
  }
};

struct NullWriter
{
  static ssize_t write(const char *data, size_t n) { return 0; }
};

class SyncLogger
{
public:
  SyncLogger() : output_(StdoutWriter::write) {}

  void setOutput(OutputFunc w) { output_ = w; }

  void produce(const char *data, size_t n) { buffer_.produce(data, n); }

  /**
   * @brief 强制将缓冲区的数据输出并重置缓冲区。
   *
   * @details 这个函数有两个主要步骤：首先根据给定的数值 n 递增消费位置，引导缓冲区准备输出数据；
   *    然后，通过 output_ 函数将缓冲区现有的可消费数据输出。最后重置整个缓冲区以便接下来的使用。
   *
   * @note 调用者需要谨慎调用此方法，因为此方法会导致缓冲区现有的所有数据被输出并且缓冲区被重置，
   *   任何尚未输出的数据将被丢弃。此外，提供的 n 值应符合预期（即不超过可消费数据的数量），
   *   否则可能会引发预期之外的行为。
   *
   * @param n 消费的位置递增的大小（字节数）。
   *
   */
  void flush(size_t n)
  {
      buffer_.incConsumablePos(n);
      output_(buffer_.data(), buffer_.consumable());
      buffer_.reset();
  }

private:
  OutputFunc output_;
  BlockingBuffer buffer_;
};

class AsyncLogger
{
public:
  AsyncLogger() : output_(StdoutWriter::write) {}

  void setOutput(OutputFunc w) { output_ = w; }

  /// TODO:
  void produce(const char *data, size_t n) {}

  /// TODO:
  void flush(size_t n) {}

private:
  OutputFunc output_;
};

template <typename Logger> class LimLog {
public:
  // 构造函数：初始化日志级别为INFO，输出为标准输出。
  LimLog() : level_(LogLevel::kInfo), output_(StdoutWriter::write) {}

  // 析构函数：删除所有的logger实例。
  ~LimLog()
  {
    for (auto l : loggers_)
      delete (l);
  }

  // 删掉拷贝构造函数和拷贝赋值操作符，确保LimLog对象不会被拷贝。
  LimLog(const LimLog &) = delete;
  LimLog &operator=(const LimLog &) = delete;

  // 向每个线程中的BlockingBuffer产生长度为 n 的数据.
  void produce(const char *data, size_t n) { logger()->produce(data, n); }

  // 刷新长度为 n 的日志行。
  void flush(size_t n) { logger()->flush(n); }

  // 设置日志级别 level。
  void setLogLevel(LogLevel level) { level_ = level; }

  // 获取当前日志级别。
  LogLevel getLogLevel() const { return level_; }

  // 设置logger输出。
  void setOutput(OutputFunc w) {
    output_ = w;
    logger()->setOutput(w);
  }

private:
  /**
 * @brief 获取当前线程的Logger实例。
 *
 * @details 此函数用于获取与当前线程关联的Logger对象。如果当前线程还没有Logger对象，则会创建一个新的Logger实例，
 * 并将其与当前线程关联。新创建的Logger实例的输出函数会被设置为类内预定义的output_函数，并且该Logger实例会被添加到loggers_列表中以便管理。
 * 使用 thread_local 关键字确保每个线程都有自己的Logger实例，这样可以避免跨线程的数据竞争问题。
 *
 * 函数首先会检查当前线程的Logger实例是否已存在，如果不存在，则使用互斥锁确保线程安全地创建并初始化一个新的Logger实例。
 * 此过程中的互斥锁保证了即使在多线程环境下，Logger实例的创建和初始化也是线程安全的。
 *
 * @note 调用此函数前不需要进行任何初始化或设置操作。但是，调用者需要确保output_函数已经被正确设置，
 * 以便新创建的Logger实例能够正确输出日志信息。创建的Logger实例将会在程序执行期间一直存在，除非手动管理其生命周期。
 *
 * @return 返回一个指向当前线程Logger实例的指针。每个线程将获得一个独立的Logger实例。
 */
  Logger *logger()
  {
    static thread_local Logger *l = nullptr;
    if (!l) {
      std::lock_guard<std::mutex> lock(loggerMutex_);
      l = static_cast<Logger *>(new Logger);
      l->setOutput(output_);
      loggers_.push_back(l);
    }
    return l;
  }

  LogLevel level_;                      // 日志级别
  OutputFunc output_;               // 输出函数
  std::mutex loggerMutex_;        // 保护logger列表的互斥锁
  std::vector<Logger *> loggers_;// 存储logger实例的容器
};

// 单例指针。
static LimLog<SyncLogger> *singleton() {
  static LimLog<SyncLogger> s_limlog;
  return &s_limlog;
}

// 日志位置，包含文件名、函数名和行号。
struct LogLoc {
public:
  // 构造函数。
  LogLoc() : LogLoc("", "", 0) {}

  // 带参构造函数，初始化文件名、函数名和行号。
  LogLoc(const char *file, const char *function, uint32_t line)
      : file_(file), function_(function), line_(line) {}

  // 判断日志位置是否为空。
  bool empty() const { return line_ == 0; }

  const char *file_;            // 文件名
  const char *function_;    // 函数名
  uint32_t line_;               // 行号
};

// 内存中的日志格式
//  +--------+-------+--------------+------+-----+--------------+------+
//  | level | time | thread id | line | file | (function) | logs |
//  +--------+-------+--------------+------+-----+--------------+------+
class LogLine {
public:
  LogLine() = delete;
  LogLine(const LogLine &) = delete;
  LogLine &operator=(const LogLine &) = delete;

  LogLine(LogLevel level, const LogLoc &loc) : count_(0), loc_(loc) {
    *this << stringifyLogLevel(level) << ' ' << Time::now().formatMilli() << ' '
          << gettid() << loc_ << ' ';
  }

  ~LogLine() {
    *this << '\n';
    singleton()->flush(count_);
  }

  /// Overloaded `operator<<` for type various of integral num.
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
  LogLine &operator<<(T v) {
    char buf[sizeof(T) * 4];
    size_t len = formatInt(v, buf);
    append(buf, len);
    return *this;
  }

  // 为bool类型重载`operator<<`。
  LogLine &operator<<(bool v) {
    if (v)
      append("true", 4);
    else
      append("false", 5);
    return *this;
  }

  // 为char类型重载`operator<<`。
  LogLine &operator<<(char v) {
    append(&v, 1);
    return *this;
  }

  // 为float类型重载`operator<<`。
  LogLine &operator<<(float v) {
    std::numeric_limits<float>::min();
    std::string s = std::to_string(v);
    append(s.data(), s.length());
    return *this;
  }

  // 为double类型重载`operator<<`。
  LogLine &operator<<(double v) {
    std::string s = std::to_string(v);
    append(s.data(), s.length());
    return *this;
  }

  // 为C风格字符串类型重载`operator<<`。
  LogLine &operator<<(const char *v) {
    append(v);
    return *this;
  }

  // 为std::string字符串类型重载`operator<<`。
  LogLine &operator<<(const std::string &v) {
    append(v.data(), v.length());
    return *this;
  }

  LogLine &operator<<(const LogLoc &loc) {
    if (!loc.empty())
      *this << ' ' << loc.file_ << ":" << loc.line_;
    return *this;
  }

private:
  void append(const char *data, size_t n) {
    singleton()->produce(data, n);
    count_ += n;
  }

  void append(const char *data) { append(data, strlen(data)); }

  size_t count_; // 日志行字节计数。
  LogLoc loc_;
};
} // namespace limlog

// 使用指定的日志级别 level 和日志位置 loc 创建一条日志行。
#define LOG(level, loc)                                                        \
  if (limlog::singleton()->getLogLevel() <= level)                             \
  limlog::LogLine(level, loc)

// 使用指定的日志级别 level 和当前的日志位置创建一条日志行。
#define LOG_LOC(level)                                                         \
  LOG(level, limlog::LogLoc(__FILE__, __FUNCTION__, __LINE__))

#define LOG_TRACE LOG_LOC(limlog::LogLevel::kTrace)
#define LOG_DEBUG LOG_LOC(limlog::LogLevel::kDebug)
#define LOG_INFO LOG_LOC(limlog::LogLevel::kInfo)
#define LOG_WARN LOG_LOC(limlog::LogLevel::kWarn)
#define LOG_ERROR LOG_LOC(limlog::LogLevel::kError)
#define LOG_FATAL LOG_LOC(limlog::LogLevel::kFatal)

/**
 * @brief 文件日志类，提供静态方法写日志到文件。
 *
 * @details 该类实现了一个简单的文件日志系统。它会确保每次只使用一个日志文件，当文件大小达到预设的最大值，
 *          或者日期变更时，当前的日志文件会被轮替(即关闭当前文件并用新的时间戳创建新文件)。
 *          如果写入失败，会将异常捕获并输出到标准错误流(stderr)。
 */
class FileLogger
{
public:
    /**
     * @brief 将数据写入日志文件。
     *
     * @details 此函数负责将传入的数据写入到日志文件中。如果日志文件未打开，
     *          或者已经达到最大文件大小，或者已经过了一天，它将执行文件轮替逻辑。
     *          如果写入日志文件失败，将抛出异常。
     *
     * @param data 指向要写入日志的数据的指针。
     * @param n 要写入的数据长度。
     * @return ssize_t 成功写入的数据字节数，如果写入失败则返回-1。
     * @note 在多线程环境中未加锁，请确保调用时处理好同步问题。
     */
    static ssize_t write(const char *data, size_t n)
    {
        static std::ofstream logFile;
        static std::string currentLogFileName = getTodayLogFileName();
        static size_t currentFileSize = std::filesystem::exists(currentLogFileName)
                                            ? std::filesystem::file_size(currentLogFileName)
                                            : 0;

        ensureLogFolderExists();
        try
        {
            ensureLogFileIsOpen(logFile, currentLogFileName, currentFileSize);
            logFile.write(data, n);
            if (!logFile.good())
            {
                throw std::runtime_error("Failed to write to log file");
            }
            currentFileSize += n;
        }
        catch (const std::exception &e)
        {
            // 如果写入文件失败，则输出到控制台
            std::cerr << "Exception caught in logger: " << e.what() << "\n";
            std::cerr << "Logging to console instead: " << std::string(data, n) << "\n";
            return -1;
        }

        return n; // 返回写入字节数
    }

private:
    /**
     * @brief 确保日志文件夹存在。
     *
     * @details 如果日志文件夹不存在，此函数会创建它。
     * @note 会抛出异常如果无法创建日志文件夹。
     */
    static void ensureLogFolderExists()
    {
        if (!std::filesystem::exists(LOG_FOLDER))
        {
            std::filesystem::create_directory(LOG_FOLDER);
        }
    }

    /**
     * @brief 确保日志文件已打开且可用。
     *
     * @details 如果日志文件未打开，或者已经到了新的一天或文件大小达到最大值，它会进行轮替，并开启新的日志文件。
     *
     * @param logFile 日志文件ofstream引用。
     * @param currentLogFileName 当前日志文件名的引用。
     * @param currentFileSize 当前日志文件大小的引用。
     * @note 如果无法打开新的日志文件会抛出异常。
     */
    static void ensureLogFileIsOpen(std::ofstream &logFile, std::string &currentLogFileName, size_t &currentFileSize)
    {
        if (!logFile.is_open() || isNextDay() || currentFileSize >= MAX_FILE_SIZE)
        {
            rollOver(logFile, currentLogFileName);
            currentFileSize = 0;                        // 重置当前文件大小计数器
            currentLogFileName = getTodayLogFileName(); // 更新当前日志文件的引用
        }
    }

    /**
     * @brief 检查是否到了新的一天。
     *
     * @details 通过比较系统时间来确定是否已经过了一天，如果是，则需要进行文件轮替。
     *
     * @return bool 如果是新的一天返回true，否则返回false。
     */
    static bool isNextDay()
    {
        static auto lastLogTime = std::chrono::system_clock::now(); // 上次记录日志的时间
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto time_t_last = std::chrono::system_clock::to_time_t(lastLogTime);

        std::tm tm_now{}, tm_last{};
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
        localtime_s(&tm_last, &time_t_last);
#else
        localtime_r(&time_t_now, &tm_now);
        localtime_r(&time_t_last, &tm_last);
#endif

        if (tm_now.tm_mday != tm_last.tm_mday || // 检查是否为新的一天
            tm_now.tm_mon != tm_last.tm_mon ||   // 同月检查
            tm_now.tm_year != tm_last.tm_year)
        {                      // 同年检查
            lastLogTime = now; // 更新最后记录时间为当前时间
            return true;       // 是新的一天
        }
        return false; // 不是新的一天
    }

    /**
     * @brief 获取当天的日志文件名。
     *
     * @details 根据当前日期生成日志文件的文件名，形式通常为"logfile_YYYYMMDD.txt"。
     *
     * @return std::string 返回当天的日志文件名。
     */
    static std::string getTodayLogFileName()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
#ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
#else
        localtime_r(&time_t_now, &tm_now);
#endif

        char buffer[64];
        strftime(buffer, sizeof(buffer), "logfile_%Y%m%d.txt", &tm_now);
        return (std::filesystem::path(LOG_FOLDER) / buffer).string();
    }

    /**
     * @brief 执行日志文件的轮替操作。
     *
     * @details 关闭当前的日志文件，并根据当前时间生成新的文件名，然后打开新的日志文件以供写入。
     *
     * @param logFile 日志文件ofstream引用，会被关闭并重新打开。
     * @param currentLogFileName 当前日志文件名的引用，轮替后会更新为新的文件名。
     * @note 如果文件重命名失败或无法打开新的日志文件，程序将输出错误信息到stderr，并可能抛出异常。
     */
    static void rollOver(std::ofstream &logFile, std::string &currentLogFileName)
    {
        if (logFile.is_open())
        {
            logFile.close(); // 关闭当前日志文件
        }

        // 重命名当前文件以反映其已经完成
        auto new_log_name = generateRolledLogFileName(currentLogFileName);
        try
        {
            std::filesystem::rename(currentLogFileName, new_log_name);
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Failed to rotate log file: " << e.what() << "\n";
        }

        // 打开新的日志文件
        logFile.open(getTodayLogFileName(), std::ofstream::out | std::ofstream::app);
        if (!logFile)
        {
            throw std::runtime_error("Unable to open new log file for the day");
        }
    }

    /**
     * @brief 生成轮替后的日志文件名。
     *
     * @details 在当前日志文件名的基础上添加时间戳，形成一个新的文件名以表示原文件的轮替。
     *
     * @param currentLogFileName 当前日志文件的文件名。
     * @return std::string 返回包含时间戳的新文件名。
     */
    static std::string generateRolledLogFileName(const std::string &currentLogFileName)
    {
        auto index = currentLogFileName.find_last_of('.');
        if (index == std::string::npos)
            index = currentLogFileName.length();
        auto rolledName = currentLogFileName.substr(0, index);
        std::tm now_tm{};

        // 添加时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
#ifdef _WIN32
        localtime_s(&now_tm, &time_t_now);
#else
        localtime_r(&time_t_now, &now_tm);
#endif

        char buffer[64];
        strftime(buffer, sizeof(buffer), "%H%M%S", &now_tm);
        rolledName.append("_").append(buffer);

        return rolledName.append(".txt"); // 返回重命名后日志文件的名称
    }
};

// 全局函数设置默认日志输出
inline void setDefaultLogOutputFunction()
{
    limlog::singleton()->setOutput(FileLogger::write);
    limlog::singleton()->setLogLevel(limlog::LogLevel::kDebug);
}

#endif // LOG_MOD_H