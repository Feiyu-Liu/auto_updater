#include <flutter/standard_method_codec.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "auto_updater.cpp"

namespace {
int failures = 0;
#define CHECK(condition) do { if (!(condition)) { \
  std::cerr << __LINE__ << ": " << #condition << "\n"; ++failures; } } while (false)

std::vector<std::wstring> calls;
void Initialize() { calls.push_back(L"initialize"); }
void SetBuildVersion(const wchar_t* version) { calls.push_back(version); }
void Cleanup() { calls.push_back(L"cleanup"); }

class RecordingResult : public flutter::MethodResult<flutter::EncodableValue> {
 public:
  std::string error;
  bool success = false;
 protected:
  void SuccessInternal(const flutter::EncodableValue*) override { success = true; }
  void ErrorInternal(const std::string& code, const std::string&,
                     const flutter::EncodableValue*) override { error = code; }
  void NotImplementedInternal() override { error = "not_implemented"; }
};

class CountingEventSink : public flutter::EventSink<flutter::EncodableValue> {
 public:
  int success_count = 0;
 protected:
  void SuccessInternal(const flutter::EncodableValue*) override { ++success_count; }
  void ErrorInternal(const std::string&, const std::string&,
                     const flutter::EncodableValue*) override {}
  void EndOfStreamInternal() override {}
};

void Validation() {
  AutoUpdater updater(Initialize, SetBuildVersion, Cleanup);
  for (const auto& value : std::vector<flutter::EncodableValue>{
           flutter::EncodableValue(), flutter::EncodableValue(42),
           flutter::EncodableValue(flutter::EncodableMap{}),
           flutter::EncodableValue(flutter::EncodableMap{
               {flutter::EncodableValue("buildVersion"), flutter::EncodableValue(42)}}),
           flutter::EncodableValue(flutter::EncodableMap{
               {flutter::EncodableValue("buildVersion"), flutter::EncodableValue("")}}),
           flutter::EncodableValue(flutter::EncodableMap{
               {flutter::EncodableValue("buildVersion"), flutter::EncodableValue(std::string("1\0x", 3))}}),
           flutter::EncodableValue(flutter::EncodableMap{
               {flutter::EncodableValue("buildVersion"), flutter::EncodableValue(std::string("\xff", 1))}})}) {
    RecordingResult result;
    HandleSetAppBuildVersion(updater, &value, result);
    CHECK(result.error == "invalid_argument");
    CHECK(!result.success);
  }
  RecordingResult absent;
  HandleSetAppBuildVersion(updater, nullptr, absent);
  CHECK(absent.error == "invalid_argument");
}

void OrderingAndUtf16() {
  calls.clear();
  {
    AutoUpdater updater(Initialize, SetBuildVersion, Cleanup);
    CHECK(updater.SetAppBuildVersion("1.0.7+17") == nullptr);
    // Last pre-init call wins; non-ASCII survives UTF-8 to UTF-16 transfer.
    CHECK(updater.SetAppBuildVersion("1.0.7+18-\xe6\xb5\x8b\xe8\xaf\x95-\xf0\x9f\x90\x9f") == nullptr);
    CHECK(calls.empty());
    updater.SetFeedURL("https://example.test/appcast.xml");
    CHECK(calls.size() == 2);
    CHECK(calls[0] == L"1.0.7+18-\u6d4b\u8bd5-\U0001f41f");
    CHECK(calls[1] == L"initialize");
    CHECK(std::string(updater.SetAppBuildVersion("1.0.8+19")) == "already_initialized");
    flutter::EncodableValue arguments(flutter::EncodableMap{
        {flutter::EncodableValue("buildVersion"), flutter::EncodableValue("1.0.8+19")}});
    RecordingResult result;
    HandleSetAppBuildVersion(updater, &arguments, result);
    CHECK(result.error == "already_initialized");
    updater.SetFeedURL("https://example.test/appcast.xml");
    CHECK(calls.size() == 2);
  }
  CHECK(calls.back() == L"cleanup");
}

void LegacyFallbackAndCallbacks() {
  calls.clear();
  AutoUpdater updater(Initialize, SetBuildVersion, Cleanup);
  updater.SetFeedURL("https://example.test/appcast.xml");
  CHECK(calls == std::vector<std::wstring>{L"initialize"});
  auto sink = std::make_unique<CountingEventSink>();
  auto* observed = sink.get();
  updater.RegisterEventSink(std::move(sink));
  updater.PostWinSparkleEvent("update-cancelled");
  CHECK(observed->success_count == 0);
}
}  // namespace

int main() {
  Validation();
  OrderingAndUtf16();
  LegacyFallbackAndCallbacks();
  std::cout << "native updater failures=" << failures << "\n";
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
