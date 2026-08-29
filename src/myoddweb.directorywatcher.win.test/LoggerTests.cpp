#include "pch.h"
#include <cstdarg>

#include "../myoddweb.directorywatcher.win/utils/Logger.h"

using myoddweb::directorywatcher::Logger;

class LoggerTestHelper
{
public:
  static std::wstring make_message(const wchar_t* format, ...)
  {
    va_list args;
    va_start(args, format);
    auto result = Logger::make_message(format, args);
    va_end(args);
    return result;
  }
};

TEST(Logger, MakeMessageSizeMatchesFormattedContent)
{
  const auto message = LoggerTestHelper::make_message(L"count=%d name=%ls", 42, L"hello");
  EXPECT_FALSE(message.empty());
  EXPECT_EQ(std::wstring(L"count=42 name=hello").size(), message.size());
  EXPECT_STREQ(L"count=42 name=hello", message.c_str());
}

TEST(Logger, MakeMessageWithNullFormatReturnsEmpty)
{
  const auto message = LoggerTestHelper::make_message(nullptr);
  EXPECT_TRUE(message.empty());
}
