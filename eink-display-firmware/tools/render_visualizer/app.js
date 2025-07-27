const express = require('express');
const { exec } = require('child_process');
const fs = require('fs');
const path = require('path');
const chokidar = require('chokidar');
const WebSocket = require('ws'); // Add WebSocket library

const app = express();
const port = 3000;
const wss = new WebSocket.Server({ port: 8080 });

// Paths
const generatorCppFilePath = path.join(__dirname, 'image_generator.cpp');
const generatorExecutablePath = path.join(__dirname, 'dist/image_generator.out');
const srcFolderPath = path.join(__dirname, '../../src/');
const outputDir = path.join(__dirname, 'dist');
const indexHtmlPath = path.join(__dirname, 'index.html');

// Dependencies
const dependencies = [
    '../../src/weatherRenderer.cpp',
    '../../src/draw/drawUtils.cpp',
]

// Ensure output directory exists
if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
}

// Set up static file serving
app.use(express.static(outputDir));

// Compile the C++ code
function compileCode() {
    return new Promise((resolve, reject) => {
        console.log('Compiling C++ code...');

        exec(`g++ ${generatorCppFilePath} ${dependencies.join(' ')} -o ${generatorExecutablePath} -std=gnu++17`, (error, stdout, stderr) => {
            if (error) {
                console.error(`Compilation error: ${error.message}`);
                return reject(error);
            }
            if (stderr) {
                console.error(`Compilation stderr: ${stderr}`);
            }
            console.log('Compilation successful');
            resolve();
        });
    });
}

// Run the C++ executable
function runExecutable() {
    return new Promise((resolve, reject) => {
        console.log('Running executable...');

        // Set the working directory to public for output files
        const options = { cwd: outputDir };

        exec(generatorExecutablePath, options, (error, stdout, stderr) => {
            if (error) {
                console.error(`Execution error: ${error.message}`);
                return reject(error);
            }
            if (stderr) {
                console.error(`Execution stderr: ${stderr}`);
            }
            console.log('Execution stdout:', stdout);
            resolve();

            // Notify clients via WebSocket
            broadcastWss(JSON.stringify({ type: 'reload-image' }));
        });
    });
}

// Process file changes
async function compileAndExecute() {
    try {
        await compileCode();
        await runExecutable();
        console.log('File processed successfully');
    } catch (error) {
        console.error('Error processing file:', error);
    }
}

// Send WebSocket message to all clients
function broadcastWss(message) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(message);
        }
    });
}

// Watch for file changes
function watchFile(filePath, callback) {
    console.log(`Watching for changes in ${path.relative(process.cwd(), filePath)}`);
    const watcher = chokidar.watch(filePath, {
        persistent: true
    });

    watcher.on('change', path => {
        console.log(`File ${path} has been changed`);
        callback();
    });
}

function copyHtml() {
    fs.copyFileSync(indexHtmlPath, path.join(outputDir, 'index.html'));
}

// Main
async function init() {

    // Initial processing
    copyHtml();
    await compileAndExecute();

    // Start watching for changes
    watchFile(srcFolderPath, compileAndExecute);
    watchFile(indexHtmlPath, copyHtml);

    // Start the servers
    app.listen(port, () => {
        console.log(`Server running at http://localhost:${port}`);
    });
    wss.on('connection', ws => {
        console.log('WebSocket client connected');
    });
}

init();