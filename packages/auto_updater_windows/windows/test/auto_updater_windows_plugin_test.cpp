#include <flutter/standard_method_codec.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "auto_updater_windows_plugin.h"

namespace auto_updater_windows {
namespace test {

namespace {

class CountingEventSink : public flutter::EventSink<flutter::EncodableValue> {
 public:
  int success_count = 0;

 protected:
  void SuccessInternal(const flutter::EncodableValue* event) override {
    success_count++;
  }

  void ErrorInternal(const std::string& error_code,
                     const std::string& error_message,
                     const flutter::EncodableValue* error_details) override {}

  void EndOfStreamInternal() override {}
};

}  // namespace

int RunDropsCallbackUntilPlatformWindowIsAvailable() {
  AutoUpdater updater;
  auto sink = std::make_unique<CountingEventSink>();
  auto* sink_pointer = sink.get();
  updater.RegisterEventSink(std::move(sink));

  updater.PostWinSparkleEvent("update-cancelled");

  return sink_pointer->success_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace test
}  // namespace auto_updater_windows

int main() {
  return auto_updater_windows::test::
      RunDropsCallbackUntilPlatformWindowIsAvailable();
}
