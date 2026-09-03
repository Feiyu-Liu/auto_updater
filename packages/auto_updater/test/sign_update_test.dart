import 'package:flutter_test/flutter_test.dart';

import '../bin/sign_update.dart';

void main() {
  test('requires an update artifact path', () {
    expect(() => signUpdate(const []), throwsArgumentError);
  });
}
