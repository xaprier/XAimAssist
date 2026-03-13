/// @file Logger.cpp
#include "core/Logger.hpp"

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace {
std::string_view toString(xaimassist::core::LogLevel level) {
    using xaimassist::core::LogLevel;

    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }

    return "UNKNOWN";
}

std::mutex g_logMutex;
std::mutex g_debugFlagMutex;
std::unordered_map<std::string, bool> g_debugFlagCache;

bool parseBoolFlagValue(const char* value) {
    if (value == nullptr) {
        return false;
    }

    std::string normalized(value);
    for (char& character : normalized) {
        character =
            static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    return normalized == "1" || normalized == "true" || normalized == "on" ||
           normalized == "yes" || normalized == "y";
}

bool readDebugFlagNoCache(const std::string& flagName) {
    if (flagName.empty()) {
        return false;
    }

    if (parseBoolFlagValue(std::getenv("DEBUG_ALL"))) {
        return true;
    }

    return parseBoolFlagValue(std::getenv(flagName.c_str()));
}
}  // namespace

namespace xaimassist::core {
Logger::Logger() : m_sink(_DefaultSink) {}

Logger::Logger(Sink sink) : m_sink(std::move(sink)) {
    if (!m_sink) {
        m_sink = _DefaultSink;
    }
}

void Logger::Log(LogLevel level, std::string_view category,
                 std::string_view message) const {
    m_sink(level, category, message, std::chrono::system_clock::now());
}

void Logger::LogIf(std::string_view debugFlagName, LogLevel level,
                   std::string_view category, std::string_view message) const {
    if (!DebugFlagEnabled(debugFlagName)) {
        return;
    }

    Log(level, category, message);
}

void Logger::Debug(std::string_view category, std::string_view message) const {
    Log(LogLevel::Debug, category, message);
}

void Logger::DebugIf(std::string_view debugFlagName, std::string_view category,
                     std::string_view message) const {
    LogIf(debugFlagName, LogLevel::Debug, category, message);
}

void Logger::Info(std::string_view category, std::string_view message) const {
    Log(LogLevel::Info, category, message);
}

void Logger::Warning(std::string_view category,
                     std::string_view message) const {
    Log(LogLevel::Warning, category, message);
}

void Logger::Error(std::string_view category, std::string_view message) const {
    Log(LogLevel::Error, category, message);
}

bool Logger::DebugFlagEnabled(std::string_view flagName) {
    if (flagName.empty()) {
        return false;
    }

    const std::string normalizedFlagName(flagName);

    std::lock_guard<std::mutex> lock(g_debugFlagMutex);
    const auto cached = g_debugFlagCache.find(normalizedFlagName);
    if (cached != g_debugFlagCache.end()) {
        return cached->second;
    }

    const bool enabled = readDebugFlagNoCache(normalizedFlagName);
    g_debugFlagCache.emplace(normalizedFlagName, enabled);
    return enabled;
}

void Logger::_DefaultSink(LogLevel level, std::string_view category,
                          std::string_view message,
                          std::chrono::system_clock::time_point timestamp) {
    const auto asTime = std::chrono::system_clock::to_time_t(timestamp);
    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &asTime);
#else
    localtime_r(&asTime, &localTime);
#endif

    std::lock_guard<std::mutex> lock(g_logMutex);
    std::clog << '[' << std::put_time(&localTime, "%F %T") << ']' << '['
              << toString(level) << ']' << '[' << category << "] " << message
              << '\n';
}
}  // namespace xaimassist::core
