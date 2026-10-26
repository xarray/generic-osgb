#!/usr/bin/env python3
"""Minimal static server for the Cesium example.

The Cesium page imports the shared three.js adapter/pager from ../threejs/ and
fetches the sample database from /data/, so the server root is the whole
``examples/`` directory:

    python3 examples/cesium/serve.py
    # then open http://localhost:8002/cesium/

Options:
    --port N    port to listen on (default 8002)
    --root DIR  directory to serve (default <repo>/examples)

Only the standard library is used.
"""

import argparse
import http.server
import os

HERE = os.path.dirname(os.path.abspath(__file__))
EXAMPLES = os.path.dirname(HERE)


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=Handler.root, **kwargs)

    def do_GET(self):
        if self.path in ('/', '/index.html'):
            self.send_response(302)
            self.send_header('Location', '/cesium/')
            self.end_headers()
            return
        super().do_GET()

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def guess_type(self, path):
        # Emscripten and Cesium both care about these MIME types.
        if path.endswith('.wasm'):
            return 'application/wasm'
        if path.endswith('.js'):
            return 'text/javascript'
        return super().guess_type(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8002)
    parser.add_argument('--root', default=EXAMPLES)
    args = parser.parse_args()

    Handler.root = os.path.abspath(args.root)
    server = http.server.ThreadingHTTPServer(('0.0.0.0', args.port), Handler)
    print(f'serving {Handler.root}')
    print(f'open http://localhost:{args.port}/cesium/')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
