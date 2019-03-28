#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <trompeloeil.hpp>

namespace trompeloeil
{
  template <>
  void reporter<specialized>::send(
    severity s,
    const char* file,
    unsigned long line,
    const char* msg)
  {
    auto f = line ? file : "[file/line unavailable]";
    if (s == severity::fatal)
    {
      ADD_FAIL_AT(f, line, msg);
    }
    else
    {
      ADD_FAIL_CHECK_AT(f, line, msg);
    }
  }
}
