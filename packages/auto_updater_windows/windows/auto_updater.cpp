#include "WinSparkle-0.8.1/include/winsparkle.h"

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <windows.h>

namespace {
// Forward declarations for WinSparkle callbacks
void __onErrorCallback();
void __onShutdownRequestCallback();
void __onDidFindUpdateCallback();
void __onDidNotFindUpdateCallback();
void __onUpdateCancelledCallback();
void __onUpdateSkippedCallback();
void __onUpdatePostponedCallback();
void __onUpdateDismissedCallback();
void __onUserRunInstallerCallback();

class AutoUpdater {
 public:
  static AutoUpdater* GetInstance();

  AutoUpdater();

  virtual ~AutoUpdater();

  void SetFeedURL(std::string feedURL);
  void CheckForUpdates();
  void CheckForUpdatesWithoutUI();
  void SetScheduledCheckInterval(int interval);

  void RegisterEventSink(
      std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> ptr);
  void OnWinSparkleEvent(std::string eventName);
  void PostWinSparkleEvent(std::string eventName);
  void SetPlatformWindow(HWND window);
  LRESULT HandlePlatformMessage(UINT message);

 private:
  static constexpr UINT kEventMessage = WM_APP + 0x4A;
  static AutoUpdater* lazySingleton;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  HWND platform_window_ = nullptr;
  std::mutex pending_events_mutex_;
  std::queue<std::string> pending_events_;
  int scheduled_check_interval_ = 0;
  bool initialized_ = false;
};

AutoUpdater* AutoUpdater::lazySingleton = nullptr;

AutoUpdater* AutoUpdater::GetInstance() {
  return lazySingleton;
}

AutoUpdater::AutoUpdater() {
  if (lazySingleton != nullptr) {
    throw std::invalid_argument("AutoUpdater has already been initialized");
  }

  lazySingleton = this;
}

AutoUpdater::~AutoUpdater() {
  if (initialized_) {
    win_sparkle_cleanup();
    initialized_ = false;
  }
  if (lazySingleton == this) {
    lazySingleton = nullptr;
  }
}

void AutoUpdater::SetFeedURL(std::string feedURL) {
  win_sparkle_set_appcast_url(feedURL.c_str());
  win_sparkle_set_automatic_check_for_updates(1);
  win_sparkle_set_error_callback(__onErrorCallback);
  win_sparkle_set_shutdown_request_callback(__onShutdownRequestCallback);
  win_sparkle_set_did_find_update_callback(__onDidFindUpdateCallback);
  win_sparkle_set_did_not_find_update_callback(__onDidNotFindUpdateCallback);
  win_sparkle_set_update_cancelled_callback(__onUpdateCancelledCallback);
  if (scheduled_check_interval_ > 0) {
    win_sparkle_set_update_check_interval(scheduled_check_interval_);
  }

  if (!initialized_) {
    win_sparkle_init();
    initialized_ = true;
  }

  // TODO: These will be supported once we update WinSparkle to >0.8.0
  // win_sparkle_set_update_skipped_callback(__onUpdateSkippedCallback);
  // win_sparkle_set_update_postponed_callback(__onUpdatePostponedCallback);
  // win_sparkle_set_update_dismissed_callback(__onUpdateDismissedCallback);
  // win_sparkle_set_user_run_installer_callback(__onUserRunInstallerCallback);
}

void AutoUpdater::CheckForUpdates() {
  win_sparkle_check_update_with_ui();
  PostWinSparkleEvent("checking-for-update");
}

void AutoUpdater::CheckForUpdatesWithoutUI() {
  win_sparkle_check_update_without_ui();
  PostWinSparkleEvent("checking-for-update");
}

void AutoUpdater::SetScheduledCheckInterval(int interval) {
  scheduled_check_interval_ = interval;
  if (initialized_) {
    win_sparkle_set_update_check_interval(interval);
  }
}

void AutoUpdater::RegisterEventSink(
    std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> ptr) {
  event_sink_ = std::move(ptr);
}

void AutoUpdater::SetPlatformWindow(HWND window) {
  platform_window_ = window;
}

LRESULT AutoUpdater::HandlePlatformMessage(UINT message) {
  if (message != kEventMessage) {
    return 0;
  }
  std::queue<std::string> events;
  {
    std::lock_guard<std::mutex> lock(pending_events_mutex_);
    std::swap(events, pending_events_);
  }
  while (!events.empty()) {
    OnWinSparkleEvent(std::move(events.front()));
    events.pop();
  }
  return 0;
}

void AutoUpdater::OnWinSparkleEvent(std::string eventName) {
  if (event_sink_ == nullptr)
    return;
  flutter::EncodableMap args = flutter::EncodableMap();
  args[flutter::EncodableValue("type")] = eventName;
  if (event_sink_) {
    event_sink_->Success(flutter::EncodableValue(args));
  }
}

void AutoUpdater::PostWinSparkleEvent(std::string eventName) {
  if (platform_window_ == nullptr) {
    OnWinSparkleEvent(std::move(eventName));
    return;
  }
  {
    std::lock_guard<std::mutex> lock(pending_events_mutex_);
    pending_events_.push(std::move(eventName));
  }
  PostMessage(platform_window_, kEventMessage, 0, 0);
}

void __onErrorCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("error");
}

void __onShutdownRequestCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("before-quit-for-update");
}

void __onDidFindUpdateCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("update-available");
}

void __onDidNotFindUpdateCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("update-not-available");
}

void __onUpdateCancelledCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("updateCancelled");
}

void __onUpdateSkippedCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("updateSkipped");
}

void __onUpdatePostponedCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("updatePostponed");
}

void __onUpdateDismissedCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("updateDismissed");
}

void __onUserRunInstallerCallback() {
  AutoUpdater* autoUpdater = AutoUpdater::GetInstance();
  if (autoUpdater == nullptr)
    return;
  autoUpdater->PostWinSparkleEvent("userRunInstaller");
}
}  // namespace
