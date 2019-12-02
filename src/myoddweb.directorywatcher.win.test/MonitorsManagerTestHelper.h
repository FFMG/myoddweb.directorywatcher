#pragma once

#include <string>
#include <vector>
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
using myoddweb::directorywatcher::EventAction;

constexpr auto TEST_TIMEOUT = 50;
constexpr auto  TEST_TIMEOUT_WAIT = 10000;

void _wait(long long ms);

class MonitorsManagerTestHelper;

MonitorsManagerTestHelper* Get(long long id);
bool Remove(long long id);
bool Add(long long id, MonitorsManagerTestHelper* mng);

class MonitorsManagerTestHelper
{
private:
  std::wstring _folder;
  std::wstring _tmpFolder;
  std::vector<std::wstring> _files;
  std::vector<std::wstring> _folders;
  int _addedFiles;
  int _addedFolders;
  int _removedFiles;
  int _removedFolders;

public:
  MonitorsManagerTestHelper();
  ~MonitorsManagerTestHelper();

  MonitorsManagerTestHelper( const MonitorsManagerTestHelper& ) = delete;
  const MonitorsManagerTestHelper&  operator=(const MonitorsManagerTestHelper&) = delete;

  auto Folder() const -> const wchar_t*;

  void EventAction(EventAction action, bool isFile);

  [[nodiscard]] auto Added(bool isFile) const -> int;
  [[nodiscard]] auto Removed(bool isFile) const -> int;

  [[nodiscard]] auto RemoveFile(const std::wstring& filename) const -> bool;
  [[nodiscard]] auto RemoveFolder(const std::wstring& folder) const -> bool;
  std::wstring AddFile();
  std::wstring AddFolder();

protected:
  static std::wstring RandomString(const size_t length);
};

inline auto function = []
(
  const long long id,
  const bool isFile,
  const wchar_t* name,
  const wchar_t* oldName,
  const int action,
  const int error,
  const long long dateTimeUtc
) -> int
{
  Get(id)->EventAction(static_cast<::EventAction>(action), isFile);
  return 0;
};
