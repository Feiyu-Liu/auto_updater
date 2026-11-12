#include <windows.h>
#include <iostream>
#include <string>
#include "../WinSparkle-0.8.1/include/winsparkle.h"

namespace {
HANDLE completed;
int outcome = 30;
void Found() { outcome = 10; SetEvent(completed); }
void NotFound() { outcome = 0; SetEvent(completed); }
void Failed() { outcome = 20; SetEvent(completed); }
}

int main(int argc, char** argv) {
  if (argc != 5) return 40;
  completed = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  const std::string identity = argv[2];
  const bool scheduled = std::string(argv[3]) == "scheduled";
  const std::string registry_path = std::string("Software\\SkillDeckUpdaterTests\\") + argv[4];
  win_sparkle_set_registry_path(registry_path.c_str());
  win_sparkle_set_appcast_url(argv[1]);
  win_sparkle_set_automatic_check_for_updates(scheduled ? 1 : 0);
  win_sparkle_set_update_check_interval(3600);
  win_sparkle_set_did_find_update_callback(Found);
  win_sparkle_set_did_not_find_update_callback(NotFound);
  win_sparkle_set_error_callback(Failed);
  if (identity != "legacy") {
    const std::wstring wide(identity.begin(), identity.end());
    win_sparkle_set_app_build_version(wide.c_str());
  }
  win_sparkle_init();
  if (!scheduled) win_sparkle_check_update_with_ui();
  const DWORD waited = WaitForSingleObject(completed, 45000);
  if (waited != WAIT_OBJECT_0) outcome = 30;
  win_sparkle_cleanup();
  CloseHandle(completed);
  std::cout << "identity=" << identity << " mode=" << argv[3]
            << " result=" << outcome << "\n";
  // Remove only the isolated fixture's unique updater preferences.
  RegDeleteTreeA(HKEY_CURRENT_USER, registry_path.c_str());
  return outcome;
}
