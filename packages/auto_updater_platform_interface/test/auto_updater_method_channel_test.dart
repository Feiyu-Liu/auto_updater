import 'package:auto_updater_platform_interface/auto_updater_platform_interface.dart';
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
}
