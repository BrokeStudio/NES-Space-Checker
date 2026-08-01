#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#define LOG_BUF_SIZE 1024

enum class LogLevel
{
  Info,
  Warn,
  Error
};

struct LogLine
{
  LogLevel level;
  std::string text;
};

class Logger
{
public:
  Logger() { clear(); }

  void addf(LogLevel lvl, const char *fmt, ...)
  {
    LogLine logLine;
    logLine.level = lvl;

    char buf[LOG_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, args);
    buf[LOG_BUF_SIZE - 1] = 0;
    va_end(args);
    logLine.text = std::string(buf);
    lines_.push_back(logLine);
  }
  const std::vector<LogLine> &lines() const { return lines_; }
  void clear() { lines_.clear(); }

private:
  std::vector<LogLine> lines_;
};
