#ifndef LOG_MOD_H
#define LOG_MOD_H

#include <string.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include <sstream>
typedef unsigned int thread_id_t;

namespace limlog {

// DigitsTable用于查找100以内的数字。
// 每两个字符对应一位数和十位数。
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

static inline size_t formatChar(char *to, char c)
{
  *to = c;
  return sizeof(char);
}

enum TimeFieldLen : size_t
{
  Year = 4,
  Month = 2,
  Day = 2,
  Hour = 2,
  Minute = 2,
  Second = 2,
};

enum SecFracLen : size_t { Sec = 0, Milli = 3, Macro = 6, Nano = 9 };

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

  // 获取时区名称和UTC东部的偏移秒数。
  std::pair<long int, std::string> timezone() const
  {
    static thread_local long int t_off = std::numeric_limits<long int>::min();
    static thread_local char t_zone[8];

    if (t_off == std::numeric_limits<long int>::min()) {
      struct tm t;
      time_t c = std::chrono::system_clock::to_time_t(tp_);
      localtime_r(&c, &t);
      t_off = t.tm_gmtoff;
      std::copy(t.tm_zone,
                t.tm_zone + std::char_traits<char>::length(t.tm_zone), t_zone);
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
  struct tm toTm() const
  {
    struct tm t;
    time_t c = std::chrono::system_clock::to_time_t(tp_);
    localtime_r(&c, &t);    // 使用 localtime_r() 以考虑本地时区

    return t;
  }

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

  size_t formatTime(char *to, int hour, int min, int sec,
                    size_t fracLen) const {
    char *p = to;
    p += formatPartialTime(p, hour, min, sec, fracLen);
    p += formatTimeOff(p);
    return p - to;
  }

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

  size_t formatSecFrac(char *to, int frac, size_t fracLen) const
  {
    if (fracLen == 0 || frac == 0)
      return 0;

    char *p = to;
    p += formatChar(p, '.');
    p += formatUIntWidth(frac, p, fracLen);
    return p - to;
  }

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

// 将日志等级转换为长度为5的字符串。
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

  // 增加可消费位置\ n 个完整日志的长度。
  void incConsumablePos(uint32_t n) {
    consumablePos_ += n;
    std::atomic_thread_fence(std::memory_order_release);
  }

  // 指向消费位置的指针。
  char *data() { return &storage_[offsetOfPos(consumePos_)]; }

  // 消费 n 字节数据并仅移动消费位置。
  void consume(uint32_t n) { consumePos_ += n; }

  // 将\ n 字节数据消费到\ to 之中。
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

  // 从 a 中复制 n 字节日志信息到缓冲区。当缓冲区空间不足时将会阻塞。
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
  // 获取当前线程的logger实例，若不存在则创建一个。
  Logger *logger() {
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
//  +-------+------+-----------+------+------+------------+------+
//  | level | time | thread id | logs | file | (function) | line |
//  +-------+------+-----------+------+------+------------+------+
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

#endif // LOG_MOD_H