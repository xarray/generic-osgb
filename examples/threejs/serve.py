#!/usr/bin/env python3
"""Minimal static server for the three.js example.

Serves this directory (index.html, js/, dist/, vendor/) and maps ``/data/`` to
the repository's ``examples/data`` so a page can fetch the sample databases:

    python3 examples/threejs/serve.py
    # then open http://localhost:8000/

Options:
    --port N    port to listen on (default 8000)
    --data DIR  directory served under /data/ (default <repo>/examples/data)

Only the standard library is used. The server exists because ES modules and
WebAssembly cannot be fetched from a file:// URL.
"""

import argparse
import http.server
import os
import posixpath
import socketserver
import urllib.parse

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))


class Handler(http.server.SimpleHTTPRequestHandler):
    data_dir = os.path.join(REPO, "examples", "data")

    def translate_path(self, path):
        # /data/... is served from the dataset directory, everything else from
        # this example directory.
        clean = urllib.parse.urlparse(path).path
        if clean == "/data" or clean.startswith("/data/"):
            rel = clean[len("/data/"):]
            parts = [p for p in posixpath.normpath(rel).split("/") if p not in ("", ".", "..")]
            return os.path.join(self.data_dir, *parts)
        return super().translate_path(path)

    def guess_type(self, path):
        # WebAssembly streaming instantiation requires application/wasm, which
        # the stdlib mimetypes table does not know about.
        if path.endswith(".wasm"):
            return "application/wasm"
        if path.endswith(".js"):
            return "text/javascript"
        return super().guess_type(path)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, fmt, *args):
        pass  # keep the console quiet; uncomment to see every request


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--data", default=os.path.join(REPO, "examples", "data"))
    args = parser.parse_args()

    Handler.data_dir = os.path.abspath(args.data)
    handler = lambda *a, **kw: Handler(*a, directory=HERE, **kw)  # noqa: E731

    with Server(("0.0.0.0", args.port), handler) as httpd:
        print(f"three.js example  : http://localhost:{args.port}/")
        print(f"serving           : {HERE}")
        print(f"/data/ maps to    : {Handler.data_dir}")
        print("Ctrl-C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print()


if __name__ == "__main__":
    main()
