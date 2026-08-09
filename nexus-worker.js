// Nexus 6.1 Web Worker wrapper
// Tương thích với interface Stockfish worker (postMessage/onmessage)
// Dùng cho PChess analysis mode

let nexusModule = null;
let nexusReady = false;
let pendingCommands = [];

// Load WASM module
importScripts('nexus6.1.js');

// Callback khi nhận output từ engine
let outputCallback = null;

// Override stdout để capture UCI output
function setupStdout() {
    if (nexusModule && nexusModule.Module) {
        nexusModule.Module.print = function(text) {
            postMessage(text);
        };
    }
}

// Khởi tạo Nexus
createNexusModule().then(function(module) {
    nexusModule = module;
    setupStdout();
    
    // Gửi UCI commands để khởi tạo
    nexusModule.ccall('main', 'int', ['int', 'string'], [0, '']);
    nexusReady = true;
    
    // Flush pending commands
    for (const cmd of pendingCommands) {
        sendCommand(cmd);
    }
    pendingCommands = [];
}).catch(function(err) {
    postMessage('ERROR: ' + (err.message || err));
});

// Nhận command từ main thread
onmessage = function(e) {
    const cmd = e.data;
    if (typeof cmd !== 'string') return;
    
    if (!nexusReady) {
        pendingCommands.push(cmd);
        return;
    }
    
    sendCommand(cmd);
};

// Gửi UCI command vào engine
function sendCommand(cmd) {
    if (!nexusModule) return;
    
    // Simulate stdin input
    // Nexus đọc từ stdin qua fgets — cần push vào buffer
    // Emscripten FS API
    if (nexusModule.FS) {
        // Write to stdin
        try {
            const stream = nexusModule.FS.open('/dev/stdin', 'w');
            nexusModule.FS.write(stream, cmd + '\n', 0, cmd.length + 1);
            nexusModule.FS.close(stream);
        } catch (e) {
            // Fallback: dùng ccall nếu có hàm processCommand
        }
    }
}
