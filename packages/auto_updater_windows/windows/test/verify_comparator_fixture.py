"""Verify the extracted comparator against a local exact upstream source file."""
import hashlib
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_bytes()
fixture = (Path(__file__).parent / 'fixtures/winsparkle_0_8_1_comparator.h').read_text()
expected = fixture.split('// Upstream source SHA-256: ', 1)[1].splitlines()[0]
assert hashlib.sha256(source).hexdigest() == expected, 'Upstream source hash differs'
text = source.decode()
start = text.index('namespace\n{', text.index('version comparison'))
end = text.index('\n\n\n', text.index('return longerResult;', start)) + 2
assert text[start:end] in fixture, 'Extracted comparator differs from upstream'
print('WinSparkle v0.8.1 comparator source verified unchanged')
