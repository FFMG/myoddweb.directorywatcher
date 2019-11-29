#include "pch.h"
#include "../myoddweb.directorywatcher.win/utils/Request.h"

using myoddweb::directorywatcher::Request;

TEST(Request, PathIsSaved) {
  {
    const auto path = L"c:\\";
    const auto request = ::Request(path, false, nullptr, 0);
    EXPECT_TRUE(wcscmp(path, request.Path()) == 0);
  }
  {
    const auto path = L"MuchLonger:\\weird\\path";
    const auto request = ::Request(path, false, nullptr, 0);
    EXPECT_TRUE(wcscmp(path, request.Path()) == 0);
  }
}

TEST(Request, RecursiveIsSaved) {
  {
    const auto request = ::Request(L"c:\\", true, nullptr, 0);
    EXPECT_TRUE( request.Recursive() );
  }
  {
    const auto request = ::Request(L"c:\\", false, nullptr, 0);
    EXPECT_FALSE(request.Recursive());
  }
}
