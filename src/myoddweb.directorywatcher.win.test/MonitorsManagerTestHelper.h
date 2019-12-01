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

  const wchar_t* Folder() const;

  void EventAction(EventAction action, bool isFile);

  int Added( bool isFile ) const;
  int Removed( bool isFile ) const;

  bool RemoveFile(const std::wstring& filename);
  bool RemoveFolder(const std::wstring& folder);
  std::wstring AddFile();
  std::wstring AddFolder();

protected:
  static std::wstring RandomString(const size_t length);
};

auto function = []
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
  Get(id)->EventAction((::EventAction)action, isFile);
  return 0;
};
