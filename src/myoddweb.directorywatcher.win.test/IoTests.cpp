#include "pch.h"

#include "..\myoddweb.directorywatcher.win\utils\Io.h"
using myoddweb::directorywatcher::Io;

TEST(Io, CombineEmptyStrings) {
  const auto lhs = L"";
  const auto rhs = L"";
  const auto expected = L"";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyRhsWithNoBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"";
  const auto expected = L"c:\\foo\\";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyRhsWithBackSlash) {
  const auto lhs = L"c:\\foo\\";
  const auto rhs = L"";
  const auto expected = L"c:\\foo\\";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyLhsWithNoBackSlash) {
  const auto lhs = L"";
  const auto rhs = L"bar";
  const auto expected = L"\\bar";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyLhsWithBackSlash) {
  const auto lhs = L"";
  const auto rhs = L"\\bar";
  const auto expected = L"\\bar";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNoBackSlashRootDrive) {
  const auto lhs = L"c:";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNoBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingBackSlashRootDrive) {
  const auto lhs = L"c:\\";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingBackSlash) {
  const auto lhs = L"c:\\foo\\";
  const auto rhs = L"\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithStartingBackSlashRootDrive) {
  const auto lhs = L"c:";
  const auto rhs = L"\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithStartingBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingAndStartingBackSlash) {
  const auto lhs = L"c:\\";
  const auto rhs = L"\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:/";
  const auto rhs = L"/foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:///";
  const auto rhs = L"///foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:///\\/\\";
  const auto rhs = L"///\\//\\\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"/foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"///foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"///\\//\\\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsEndingBackSlash) {
  const auto lhs = L"c:/";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsEndingBackSlash) {
  const auto lhs = L"c:///";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsEndingBackSlash) {
  const auto lhs = L"c:///\\/\\";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::Combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}
