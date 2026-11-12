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

    platform.emit('update-finished');
    await pumpEventQueue();

    expect(listener.finished, isTrue);
  });

  test('returns how the native updater answered a check', () async {
    final platform = _FakeAutoUpdaterPlatform()
      ..checkResult = UpdateCheckStart.busy;
    AutoUpdaterPlatform.instance = platform;
    addTearDown(platform.dispose);

    expect(
      await autoUpdater.checkForUpdates(inBackground: true),
      UpdateCheckStart.busy,
    );
    expect(platform.lastInBackground, isTrue);
  });
}

final class _FakeAutoUpdaterPlatform extends AutoUpdaterPlatform {
  final _events = StreamController<Map<Object?, Object?>>.broadcast();
  UpdateCheckStart checkResult = UpdateCheckStart.started;
  bool? lastInBackground;

  @override
  Stream<Map<Object?, Object?>> get sparkleEvents => _events.stream;

  @override
  Future<UpdateCheckStart> checkForUpdates({bool? inBackground}) async {
    lastInBackground = inBackground;
    return checkResult;
  }

  void emit(String type) => _events.add({'type': type});

  Future<void> dispose() => _events.close();
}

final class _RecordingListener with UpdaterListener {
  bool cancelled = false;
  bool finished = false;

  @override
  void onUpdaterUpdateCancelled() {
    cancelled = true;
  }

  @override
  void onUpdaterUpdateFinished(UpdaterError? error) {
    finished = true;
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
