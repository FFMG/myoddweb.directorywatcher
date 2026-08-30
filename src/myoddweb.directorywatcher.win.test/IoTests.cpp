#include "pch.h"
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../myoddweb.directorywatcher.win/utils/Io.h"
using myoddweb::directorywatcher::Io;

TEST(Io, CombineEmptyStrings) {
  const auto lhs = L"";
  const auto rhs = L"";
  const auto expected = L"";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyRhsWithNoBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"";
  const auto expected = L"c:\\foo\\";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyRhsWithBackSlash) {
  const auto lhs = L"c:\\foo\\";
  const auto rhs = L"";
  const auto expected = L"c:\\foo\\";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyLhsWithNoBackSlash) {
  const auto lhs = L"";
  const auto rhs = L"bar";
  const auto expected = L"\\bar";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineEmptyLhsWithBackSlash) {
  const auto lhs = L"";
  const auto rhs = L"\\bar";
  const auto expected = L"\\bar";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNoBackSlashRootDrive) {
  const auto lhs = L"c:";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNoBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingBackSlashRootDrive) {
  const auto lhs = L"c:\\";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingBackSlash) {
  const auto lhs = L"c:\\foo\\";
  const auto rhs = L"\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithStartingBackSlashRootDrive) {
  const auto lhs = L"c:";
  const auto rhs = L"\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithStartingBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithEndingAndStartingBackSlash) {
  const auto lhs = L"c:\\";
  const auto rhs = L"\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:/";
  const auto rhs = L"/foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:///";
  const auto rhs = L"///foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsEndingAndStartingBackSlash) {
  const auto lhs = L"c:///\\/\\";
  const auto rhs = L"///\\//\\\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"/foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"///foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsStartingBackSlash) {
  const auto lhs = L"c:";
  const auto rhs = L"///\\//\\\\foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithNonWindowsEndingBackSlash) {
  const auto lhs = L"c:/";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleNonWindowsEndingBackSlash) {
  const auto lhs = L"c:///";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, CombineWithMultipleWindowsAndNonWindowsEndingBackSlash) {
  const auto lhs = L"c:///\\/\\";
  const auto rhs = L"foo\\bar.txt";
  const auto expected = L"c:\\foo\\bar.txt";
  const auto actual = ::Io::combine(lhs, rhs);
  ASSERT_STREQ(expected, actual.c_str());
}

TEST(Io, RootFoldersAreSame) {
  const auto lhs = L"c:\\";
  const auto rhs = L"c:\\";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, RootFoldersAreSameCaseCompare) {
  const auto lhs = L"c:\\";
  const auto rhs = L"C:\\";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersAreSame) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersAreSameCaseCompare) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"C:\\FOO";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasBackSlash) {
  const auto lhs = L"c:\\foo\\";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:\\foo\\";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasForwardSlash) {
  const auto lhs = L"c:/foo//";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasForwardSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:/foo/";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersAreNotSame) {
  const auto lhs = L"c:\\bar";
  const auto rhs = L"c:\\foo";
  ASSERT_FALSE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersAreNotSameWithBackSlash) {
  const auto lhs = L"c:\\bar";
  const auto rhs = L"c:\\foo";
  ASSERT_FALSE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasMultipleBackSlashAtEnd) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:\\foo\\\\\\";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasMultipleBackSlashAtEnd) {
  const auto lhs = L"c:\\foo\\\\\\";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasMultipleForwardSlashAtEnd) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:/foo///";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasMultipleForwardSlashAtEnd) {
  const auto lhs = L"c:/foo////";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasMultipleBackSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:\\\\\\foo\\\\\\";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasMultipleBackSlash) {
  const auto lhs = L"c:\\\\\\foo\\\\\\";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithRhsHasMultipleForwardSlash) {
  const auto lhs = L"c:\\foo";
  const auto rhs = L"c:////foo///";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

TEST(Io, FoldersWithLhsHasMultipleForwardSlash) {
  const auto lhs = L"c:/foo////";
  const auto rhs = L"c:\\foo";
  ASSERT_TRUE(::Io::are_same_folders(lhs, rhs));
}

namespace
{
  class TempFolderHelper final
  {
  public:
    TempFolderHelper()
    {
      const auto tempRoot = std::filesystem::temp_directory_path();
      _path = (tempRoot / (L"dw_io_test_" + std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()))).wstring();
      std::filesystem::create_directories(_path);
    }

    ~TempFolderHelper()
    {
      std::error_code ec;
      std::filesystem::remove_all(_path, ec);
    }

    [[nodiscard]] const std::wstring& path() const
    {
      return _path;
    }

    void create_file(const std::wstring& relativePath) const
    {
      const auto fullPath = std::filesystem::path(_path) / relativePath;
      std::filesystem::create_directories(fullPath.parent_path());
      std::ofstream file(fullPath.c_str());
      file << "test" << std::endl;
      file.close();
    }

    void create_directory(const std::wstring& relativePath) const
    {
      const auto fullPath = std::filesystem::path(_path) / relativePath;
      std::filesystem::create_directories(fullPath);
    }

  private:
    std::wstring _path;
  };
}

TEST(Io, GetAllFilesAndFoldersNonExistent) {
  const auto items = ::Io::get_all_files_and_folders(L"C:\\non_existent_folder_987654321\\", true);
  EXPECT_TRUE(items.empty());
}

TEST(Io, GetAllFilesAndFoldersEmptyFolder) {
  TempFolderHelper helper;
  const auto items = ::Io::get_all_files_and_folders(helper.path(), true);
  EXPECT_TRUE(items.empty());
}

TEST(Io, GetAllFilesAndFoldersFlat) {
  TempFolderHelper helper;
  helper.create_file(L"file1.txt");
  helper.create_file(L"file2.txt");
  helper.create_directory(L"sub_empty");

  const auto items = ::Io::get_all_files_and_folders(helper.path(), false);
  EXPECT_EQ(3, items.size());

  auto fileCount = 0;
  auto folderCount = 0;
  for (const auto& item : items)
  {
    if (item.second)
    {
      ++fileCount;
    }
    else
    {
      ++folderCount;
    }
  }

  EXPECT_EQ(2, fileCount);
  EXPECT_EQ(1, folderCount);
}

TEST(Io, GetAllFilesAndFoldersRecursive) {
  TempFolderHelper helper;
  helper.create_file(L"file1.txt");
  helper.create_directory(L"sub1");
  helper.create_file(L"sub1\\file2.txt");
  helper.create_directory(L"sub1\\nested");
  helper.create_file(L"sub1\\nested\\file3.txt");

  const auto items = ::Io::get_all_files_and_folders(helper.path(), true);
  EXPECT_EQ(5, items.size()); // 3 files + 2 folders

  auto fileCount = 0;
  auto folderCount = 0;
  for (const auto& item : items)
  {
    if (item.second)
    {
      ++fileCount;
    }
    else
    {
      ++folderCount;
    }
  }

  EXPECT_EQ(3, fileCount);
  EXPECT_EQ(2, folderCount);
}

TEST(Io, GetAllFilesAndFoldersNonRecursiveWithNested) {
  TempFolderHelper helper;
  helper.create_file(L"file1.txt");
  helper.create_directory(L"sub1");
  helper.create_file(L"sub1\\file2.txt");

  const auto items = ::Io::get_all_files_and_folders(helper.path(), false);
  EXPECT_EQ(2, items.size()); // only top-level file1.txt and sub1 folder
}
