import 'dart:async';

import 'package:auto_updater/auto_updater.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('forwards the native Windows cancellation event', () async {
    final platform = _FakeAutoUpdaterPlatform();
    AutoUpdaterPlatform.instance = platform;
    final listener = _RecordingListener();
    autoUpdater.addListener(listener);
    addTearDown(() async {
      autoUpdater.removeListener(listener);
      await platform.dispose();
    });

    platform.emit('update-cancelled');
    await pumpEventQueue();

    expect(listener.cancelled, isTrue);
  });
}

final class _FakeAutoUpdaterPlatform extends AutoUpdaterPlatform {
  final _events = StreamController<Map<Object?, Object?>>.broadcast();

  @override
  Stream<Map<Object?, Object?>> get sparkleEvents => _events.stream;

  void emit(String type) => _events.add({'type': type});

  Future<void> dispose() => _events.close();
}

final class _RecordingListener with UpdaterListener {
  bool cancelled = false;

  @override
  void onUpdaterUpdateCancelled() {
    cancelled = true;
  }

  @override
  void onUpdaterBeforeQuitForUpdate(AppcastItem? appcastItem) {}

  @override
  void onUpdaterCheckingForUpdate(Appcast? appcast) {}

  @override
  void onUpdaterError(UpdaterError? error) {}

  @override
  void onUpdaterUpdateAvailable(AppcastItem? appcastItem) {}

  @override
  void onUpdaterUpdateDownloaded(AppcastItem? appcastItem) {}

  @override
  void onUpdaterUpdateNotAvailable(UpdaterError? error) {}
}
