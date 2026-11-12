import 'package:auto_updater_platform_interface/auto_updater_platform_interface.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  final platform = MethodChannelAutoUpdater();
  final calls = <MethodCall>[];
  Object? reply;

  setUp(() {
    calls.clear();
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(platform.methodChannel, (call) async {
      calls.add(call);
      return reply;
    });
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(platform.methodChannel, null);
  });

  test('reports how the native updater answered a check', () async {
    for (final (native, expected) in [
      ('started', UpdateCheckStart.started),
      ('resumed', UpdateCheckStart.resumed),
      ('busy', UpdateCheckStart.busy),
    ]) {
      reply = native;
      expect(await platform.checkForUpdates(), expected);
    }
  });

  test('treats replies from older native implementations as started', () async {
    for (final native in <Object?>[true, null]) {
      reply = native;
      expect(await platform.checkForUpdates(), UpdateCheckStart.started);
    }
  });

  test('forwards whether the check runs in the background', () async {
    reply = 'busy';
    await platform.checkForUpdates(inBackground: true);
    await platform.checkForUpdates();

    expect(calls.map((call) => call.arguments), [
      {'inBackground': true},
      {'inBackground': false},
    ]);
  });

  test('awaits Windows build identity before initialization', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.windows;
    addTearDown(() => debugDefaultTargetPlatformOverride = null);
    await platform.setAppBuildVersion('1.0.7+18');
    await platform.setScheduledCheckInterval(3600);
    await platform.setFeedURL('https://example.test/appcast.xml');
    expect(calls.map((call) => call.method), [
      'setAppBuildVersion',
      'setScheduledCheckInterval',
      'setFeedURL',
    ]);
    expect(calls.first.arguments, {'buildVersion': '1.0.7+18'});
  });

  test('preserves native structured build identity errors', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.windows;
    addTearDown(() => debugDefaultTargetPlatformOverride = null);
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(platform.methodChannel, (call) async {
      throw PlatformException(code: 'already_initialized');
    });
    expect(
      platform.setAppBuildVersion('1.0.7+18'),
      throwsA(
        isA<PlatformException>()
            .having((e) => e.code, 'code', 'already_initialized'),
      ),
    );
  });

  test('macOS existing calls remain compatible and setter is unsupported',
      () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.macOS;
    addTearDown(() => debugDefaultTargetPlatformOverride = null);
    await expectLater(
      platform.setAppBuildVersion('1.0.7+18'),
      throwsUnsupportedError,
    );
    await platform.setFeedURL('https://example.test/appcast.xml');
    expect(calls.map((call) => call.method), ['setFeedURL']);
  });
}
