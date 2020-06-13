#include "pch.h"
#include "../myoddweb.directorywatcher.win/utils/Request.h"
#include "RequestTestHelper.h"

using myoddweb::directorywatcher::Request;

TEST(Request, PathIsSaved) {
  {
    const auto path = L"c:\\";

    // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
    const auto r = RequestHelper(
      path,
      false,
      nullptr,
      nullptr,
      nullptr,
      0,
      0);

    const auto request = ::Request(r);
    EXPECT_TRUE(wcscmp(path, request.Path()) == 0);
  }
  {
    const auto path = L"MuchLonger:\\weird\\path";

    // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
    const auto r = RequestHelper(
      path,
      false,
      nullptr,
      nullptr,
      nullptr,
      0,
      0);
    const auto request = ::Request(r);
    EXPECT_TRUE(wcscmp(path, request.Path()) == 0);
  }
}

TEST(Request, RecursiveIsSaved) {
  {
    // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
    const auto r = RequestHelper(
      L"c:\\",
      true,
      nullptr,
      nullptr,
      nullptr,
      0,
      0);
    const auto request = ::Request(r);
    EXPECT_TRUE( request.Recursive() );
  }
  {
    // use the test request to create the Request
    // we make a copy of our helper onto the 'real' request to make sure copy is not broken
    const auto r = RequestHelper(
      L"c:\\",
      false,
      nullptr,
      nullptr,
      nullptr,
      0,
      0);
    const auto request = ::Request(r);
    EXPECT_FALSE(request.Recursive());
  }
}
