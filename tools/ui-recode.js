const fs = require('fs');
const path = require('path');
const execSync = require('child_process').execSync;

// Function to update JSON files
function updateJson(filePath) {
    let content = JSON.parse(fs.readFileSync(filePath, 'utf-8'));

    // Example: Add or update Windows 11 specific styles or settings
    content.theme = 'Windows 11';
    content.borderRadius = '8px';
    content.shadow = '0px 4px 6px rgba(0, 0, 0, 0.1)';

    fs.writeFileSync(filePath, JSON.stringify(content, null, 2));
    console.log(`Updated JSON file: ${filePath}`);
}

// Function to update HEXA files
function updateHexa(filePath) {
    let content = fs.readFileSync(filePath, 'utf-8');

    // Example: Recode .hexa file format (this will depend on your specific file format)
    content = content.replace(/"theme": ".*?"/, '"theme": "Windows 11"');
    content = content.replace(/"borderRadius": ".*?"/, '"borderRadius": "8px"');

    fs.writeFileSync(filePath, content);
    console.log(`Updated HEXA file: ${filePath}`);
}

// Function to handle build directory navigation and command execution
function runBuildCommand() {
    try {
        console.log('Navigating to build directory...');
        execSync('cd build && npm install', { stdio: 'inherit' });
        console.log('Build command executed successfully');
    } catch (error) {
        console.error('Error during build command execution: ', error.message);
        process.exit(1);
    }
}

// Main function to process files
function recodeUi(filePath, theme) {
    const ext = path.extname(filePath).toLowerCase();

    // Process only .hexa and .json files
    try {
        if (ext === '.json') {
            updateJson(filePath);
        } else if (ext === '.hexa') {
            updateHexa(filePath);
        } else {
            console.log(`Ignoring unsupported file type: ${filePath}`);
        }
    } catch (err) {
        console.error(`Error processing file ${filePath}: ${err.message}`);
    }
}

// Entry point
const args = process.argv.slice(2);
if (args.length === 0 || !args[0].startsWith('--path')) {
    console.error('Usage: node ui-recode.js --path <file-path> --theme <theme>');
    process.exit(1);
}

const filePath = args[0].split('=')[1];
if (!fs.existsSync(filePath)) {
    console.error(`File not found: ${filePath}`);
    process.exit(1);
}

console.log(`Recoding UI for: ${filePath}`);
recodeUi(filePath, 'Windows 11');

// Run build commands in the build directory
runBuildCommand();
