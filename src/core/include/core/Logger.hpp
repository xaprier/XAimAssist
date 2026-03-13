/**
 * @file Logger.hpp
 * @brief Lightweight categorised logging with optional debug-flag gating.
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <chrono>
#include <functional>
#include <string_view>

namespace xaimassist::core {

/// Severity levels for log messages.
enum class LogLevel { Debug,
                      Info,
                      Warning,
                      Error };

/**
 * @class Logger
 * @brief Dispatches categorised log messages to a configurable sink.
 *
 * Debug messages can be gated by named flags (environment variable
 * XAIMASSIST_DEBUG) so verbose subsystem logs stay silent by default.
 */
class Logger {
  public:
    /// Callback signature for custom log sinks.
    using Sink = std::function<void(
        LogLevel level, std::string_view category, std::string_view message,
        std::chrono::system_clock::time_point timestamp)>;

    /// Constructs with the default stderr sink.
    Logger();

    /// Constructs with a user-provided sink.
    explicit Logger(Sink sink);

    /// Emit a log message at the given severity level.
    void Log(LogLevel level, std::string_view category,
             std::string_view message) const;

    /// Log only when the named debug flag is enabled.
    void LogIf(std::string_view debugFlagName, LogLevel level,
               std::string_view category, std::string_view message) const;

    /// Unconditional debug-level log.
    void Debug(std::string_view category, std::string_view message) const;
    /// Debug-level log gated by a named flag.
    void DebugIf(std::string_view debugFlagName, std::string_view category,
                 std::string_view message) const;
    /// Informational log.
    void Info(std::string_view category, std::string_view message) const;
    /// Warning-level log.
    void Warning(std::string_view category, std::string_view message) const;
    /// Error-level log.
    void Error(std::string_view category, std::string_view message) const;

    /// Check whether a debug flag is active via environment variable.
    static bool DebugFlagEnabled(std::string_view flagName);

  private:
    static void _DefaultSink(LogLevel level, std::string_view category,
                             std::string_view message,
                             std::chrono::system_clock::time_point timestamp);

    Sink m_sink;
};
}  // namespace xaimassist::core

#endif  // LOGGER_HPP
