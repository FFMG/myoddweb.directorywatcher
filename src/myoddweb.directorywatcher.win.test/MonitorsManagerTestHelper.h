#pragma once

#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
using myoddweb::directorywatcher::EventAction;

constexpr auto TEST_TIMEOUT = 50;
constexpr auto  TEST_TIMEOUT_WAIT = 1000;

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

  [[nodiscard]] auto Folder() const -> const wchar_t*;

  void EventAction(EventAction action, bool isFile);

  static void LoggerFunction(long long id, int type, const wchar_t* message);

  [[nodiscard]] auto Added(bool isFile) const -> int;
  [[nodiscard]] auto Removed(bool isFile) const -> int;

  [[nodiscard]] auto RemoveFile(const std::wstring& filename) const -> bool;
  [[nodiscard]] auto RemoveFolder(const std::wstring& folder) const -> bool;
  std::wstring AddFile();
  std::wstring AddFolder();

protected:
  static std::wstring RandomString(const size_t length);
};

inline auto eventFunction = []
(
  const long long id,
  const bool isFile,
  const wchar_t* name,
  const wchar_t* oldName,
  const int action,
  const int error,
  const long long dateTimeUtc
) -> void
{
  Get(id)->EventAction(static_cast<::EventAction>(action), isFile);
};

inline auto loggerFunction = []
(
  const long long id,
  const int type,
  const wchar_t* message
  ) -> void
{ 
  MonitorsManagerTestHelper::LoggerFunction(id, type, message);
};