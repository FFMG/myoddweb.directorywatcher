#pragma once

#include <string>
#include <vector>
#include "../myoddweb.directorywatcher.win/utils/EventAction.h"
using myoddweb::directorywatcher::EventAction;

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
  int _added;
  int _removed;

public:
  MonitorsManagerTestHelper();
  ~MonitorsManagerTestHelper();

  void Wait(long long ms);

  const wchar_t* Folder() const;

  void EventAction(EventAction action);

  int Added() const;

  int Removed() const;

  bool RemoveFile(const std::wstring& filename) const;

  std::wstring AddFile() const;

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
  Get(id)->EventAction((EventAction)action);
  return 0;
};
