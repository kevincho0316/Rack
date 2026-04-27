#pragma once
#include <string>
#ifdef _WIN32
  #include <io.h>
  #include <windows.h>
  namespace Color {
    inline bool enabled() {
      static bool e = []() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode))
          SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        return _isatty(_fileno(stdout)) != 0;
      }();
      return e;
    }
  }
#else
  #include <unistd.h>
  namespace Color {
    inline bool enabled() {
      static bool e = (isatty(STDOUT_FILENO) != 0);
      return e;
    }
  }
#endif

namespace Color {
  inline std::string r(const std::string& s)   { return enabled() ? "\033[31m" + s + "\033[0m" : s; }
  inline std::string g(const std::string& s)   { return enabled() ? "\033[32m" + s + "\033[0m" : s; }
  inline std::string y(const std::string& s)   { return enabled() ? "\033[33m" + s + "\033[0m" : s; }
  inline std::string m(const std::string& s)   { return enabled() ? "\033[35m" + s + "\033[0m" : s; }
  inline std::string c(const std::string& s)   { return enabled() ? "\033[36m" + s + "\033[0m" : s; }
  inline std::string b(const std::string& s)   { return enabled() ? "\033[1m"  + s + "\033[0m" : s; }
  inline std::string dim(const std::string& s) { return enabled() ? "\033[90m" + s + "\033[0m" : s; }
  inline std::string rb(const std::string& s)  { return enabled() ? "\033[1;31m" + s + "\033[0m" : s; }
  inline std::string gb(const std::string& s)  { return enabled() ? "\033[1;32m" + s + "\033[0m" : s; }
}
