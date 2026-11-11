"""Exercise the actual WinSparkle DLL against isolated loopback appcasts.

No payload is downloaded or installed. Each process has a unique temporary
updater registry path, deleted by that fixture process after native cleanup.
"""
import argparse
from functools import partial
import http.server
from pathlib import Path
import subprocess
import tempfile
import threading
import uuid


parser = argparse.ArgumentParser()
parser.add_argument('--fixture-dir', type=Path, required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='winsparkle-identity-') as temporary:
    root = Path(temporary)
    handler = partial(http.server.SimpleHTTPRequestHandler, directory=temporary)
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        cases = []
        for mode in ('manual', 'scheduled'):
            for resource in ('high', 'low'):
                cases.extend([
                    (resource, '1.0.7+18', '1.0.7+18', mode, 0),
                    (resource, '1.0.6+17', '1.0.7+18', mode, 10),
                    (resource, '1.0.8+19', '1.0.7+18', mode, 0),
                ])
        cases.extend([
            ('low', 'legacy', '1.0.7+18', 'manual', 10),
            ('high', 'legacy', '1.0.7+18', 'manual', 0),
        ])
        for resource, installed, feed, mode, expected in cases:
            feed_name = uuid.uuid4().hex + '.xml'
            (root / feed_name).write_text(f'''<?xml version="1.0"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
<channel><title>Identity fixture</title><item><title>Fixture {feed}</title>
<sparkle:version>{feed}</sparkle:version><sparkle:shortVersionString>1.0.7</sparkle:shortVersionString>
<enclosure url="http://127.0.0.1:{server.server_port}/never-download.exe"
 sparkle:version="{feed}" sparkle:shortVersionString="1.0.7"
 sparkle:os="windows" length="1" type="application/octet-stream"/>
</item></channel></rss>''', encoding='utf-8')
            command = [str(args.fixture_dir / f'winsparkle_identity_{resource}.exe'),
                       f'http://127.0.0.1:{server.server_port}/{feed_name}',
                       installed, mode, uuid.uuid4().hex]
            result = subprocess.run(command, capture_output=True, text=True, timeout=55)
            print(f'resource={resource} feed={feed} {result.stdout.strip()}', flush=True)
            if result.returncode != expected:
                raise RuntimeError(f'Expected {expected}, got {result.returncode}: {result.stderr}')
        print(f'WinSparkle DLL identity acceptance: {len(cases)}/{len(cases)} passed', flush=True)
    finally:
        server.shutdown()
        server.server_close()
