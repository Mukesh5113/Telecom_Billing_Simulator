// Minimal zero-dependency Node.js backend for the Telecom Billing Simulator UI.
//
// It serves the static front-end and exposes /api/bill, which shells out to the
// compiled `tbs` binary (--format json) and returns the parsed result. This is
// the bridge between the browser UI and the real C++ simulator.

'use strict';

const http = require('http');
const { spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const url = require('url');

const ROOT = path.resolve(__dirname, '..');
const PUBLIC = path.join(__dirname, 'public');
const SAMPLE_DIR = path.join(__dirname, 'sample');
const PORT = process.env.PORT || 3000;
const DATA_DIR = process.env.TBS_DATA || path.join(ROOT, 'data');

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
};

// Find the compiled simulator binary across common build layouts.
function locateBinary() {
  if (process.env.TBS_BIN && fs.existsSync(process.env.TBS_BIN)) {
    return process.env.TBS_BIN;
  }
  const candidates = [
    path.join(ROOT, 'build', 'tbs'),
    path.join(ROOT, 'build', 'tbs.exe'),
    path.join(ROOT, 'build', 'Release', 'tbs.exe'),
    path.join(ROOT, 'build', 'Debug', 'tbs.exe'),
  ];
  return candidates.find(fs.existsSync) || null;
}

// When the C++ binary hasn't been built, fall back to bundled sample output
// so the UI is still fully demoable. The numbers match what the simulator
// produces for the sample dataset in data/.
function loadSample(mode) {
  const file = path.join(SAMPLE_DIR, mode + '.json');
  try {
    return JSON.parse(fs.readFileSync(file, 'utf8'));
  } catch (e) {
    return null;
  }
}

// Run the simulator for one mode and parse its JSON stdout.
function runSimulator(mode, account) {
  const bin = locateBinary();
  if (!bin) {
    const data = loadSample(mode);
    if (!data) {
      return {
        ok: false,
        status: 503,
        error:
          'Simulator binary not found and no bundled sample available. Build it first:\n' +
          '  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release\n' +
          '  cmake --build build --config Release\n' +
          'Or set TBS_BIN to the tbs executable path.',
      };
    }
    return { ok: true, status: 200, data, binary: '(bundled sample)', sample: true };
  }

  const args = ['--data', DATA_DIR, '--mode', mode, '--format', 'json'];
  if (account && mode === 'postpaid') {
    args.push('--account', account);
  }

  const res = spawnSync(bin, args, { encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
  if (res.error) {
    return { ok: false, status: 500, error: 'Failed to run simulator: ' + res.error.message };
  }
  if (res.status !== 0) {
    return {
      ok: false,
      status: 500,
      error: 'Simulator exited with code ' + res.status + '\n' + (res.stderr || ''),
    };
  }

  try {
    const data = JSON.parse((res.stdout || '').trim());
    return { ok: true, status: 200, data, binary: bin };
  } catch (e) {
    return {
      ok: false,
      status: 500,
      error: 'Could not parse simulator output as JSON: ' + e.message,
    };
  }
}

function sendJson(res, status, obj) {
  const body = JSON.stringify(obj);
  res.writeHead(status, { 'Content-Type': 'application/json; charset=utf-8' });
  res.end(body);
}

function serveStatic(res, reqPath) {
  let rel = reqPath === '/' ? '/index.html' : reqPath;
  // prevent path traversal
  const safe = path.normalize(rel).replace(/^(\.\.[/\\])+/, '');
  const file = path.join(PUBLIC, safe);
  if (!file.startsWith(PUBLIC)) {
    res.writeHead(403);
    res.end('Forbidden');
    return;
  }
  fs.readFile(file, (err, buf) => {
    if (err) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('Not found');
      return;
    }
    const ext = path.extname(file).toLowerCase();
    res.writeHead(200, { 'Content-Type': MIME[ext] || 'application/octet-stream' });
    res.end(buf);
  });
}

const server = http.createServer((req, res) => {
  const parsed = url.parse(req.url, true);

  if (parsed.pathname === '/api/bill') {
    const mode = (parsed.query.mode || 'postpaid').toString();
    const account = (parsed.query.account || '').toString().trim();

    if (mode !== 'postpaid' && mode !== 'prepaid') {
      return sendJson(res, 400, { error: "mode must be 'postpaid' or 'prepaid'" });
    }
    const result = runSimulator(mode, account);
    if (!result.ok) {
      return sendJson(res, result.status, { error: result.error });
    }
    let data = result.data;
    // For bundled sample output, apply the account filter in JS (the CLI does
    // this itself when the real binary runs).
    if (result.sample && mode === 'postpaid' && account && data && data.invoices) {
      data = { invoices: data.invoices.filter((inv) => String(inv.ban) === account) };
    }
    return sendJson(res, 200, {
      mode,
      binary: result.binary,
      sample: !!result.sample,
      result: data,
    });
  }

  if (parsed.pathname === '/api/health') {
    return sendJson(res, 200, {
      status: 'ok',
      binary: locateBinary(),
      dataDir: DATA_DIR,
    });
  }

  serveStatic(res, parsed.pathname);
});

server.listen(PORT, () => {
  const bin = locateBinary();
  console.log('Telecom Billing Simulator UI running at http://localhost:' + PORT);
  console.log('  data dir : ' + DATA_DIR);
  console.log('  binary   : ' + (bin || 'NOT FOUND (build the project first)'));
});
