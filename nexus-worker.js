// Nexus 6.1 Web Worker wrapper (pump API)
// Stockfish-compatible interface (postMessage/onmessage) for PChess analysis mode.
//
// The engine uses Emscripten pthreads, so the page must be crossOriginIsolated
// (SharedArrayBuffer). Serve the worker page with:
//   Cross-Origin-Opener-Policy: same-origin
//   Cross-Origin-Embedder-Policy: require-corp
//
// Command flow: postMessage("position startpos ...") -> nexus_push_command()
// queues the line; nexus_pump() drains it through HandleCommand(). "go" wakes
// the search pthread and returns immediately; bestmove arrives later via
// Module.print. "quit" -> nexus_destroy() shuts worker threads down.

let nexusModule = null;
let nexusReady = false;
let pendingCommands = [];

importScripts('nexus6.1.js');

// Engine stdout (printf) is forwarded here by Emscripten's Module.print.
function onEngineOutput(text) {
    postMessage(text);
}

createNexusModule({
    print: onEngineOutput,
    printErr: onEngineOutput,
}).then(function(module) {
    nexusModule = module;
    module._nexus_init();
    nexusReady = true;
    for (const cmd of pendingCommands) {
        pushCommand(cmd);
    }
    pendingCommands = [];
    module._nexus_pump();
}).catch(function(err) {
    postMessage('ERROR: ' + (err.message || err));
});

onmessage = function(e) {
    const cmd = e.data;
    if (typeof cmd !== 'string') return;
    if (!nexusReady) {
        pendingCommands.push(cmd);
        return;
    }
    pushCommand(cmd);
    nexusModule._nexus_pump();
};

function pushCommand(cmd) {
    if (!nexusModule) return;
    const len = nexusModule.lengthBytesUTF8(cmd);
    const ptr = nexusModule._malloc(len + 1);
    nexusModule.stringToUTF8(cmd, ptr, len + 1);
    const ok = nexusModule._nexus_push_command(ptr);
    nexusModule._free(ptr);
    if (!ok) {
        onEngineOutput('ERROR: command buffer full — command dropped');
    }
}
