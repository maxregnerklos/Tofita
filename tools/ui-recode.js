const fs = require('fs');
const path = require('path');

// Function to update JSON files
function updateJson(filePath) {
    let content = JSON.parse(fs.readFileSync(filePath, 'utf-8'));

    // Example: Add or update Windows 11 specific styles
    content.theme = 'Windows 11';
    content.borderRadius = '8px';
    content.shadow = '0px 4px 6px rgba(0, 0, 0, 0.1)';

    fs.writeFileSync(filePath, JSON.stringify(content, null, 2));
    console.log(`Updated JSON file: ${filePath}`);
}

// Function to update CSS files
function updateCss(filePath) {
    let content = fs.readFileSync(filePath, 'utf-8');

    // Example: Append Windows 11 styles
    const windows11Styles = `
/* Windows 11 Styling */
:root {
    --primary-color: #0078D4;
    --border-radius: 8px;
    --shadow: 0px 4px 6px rgba(0, 0, 0, 0.1);
}
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background-color: #F3F3F3;
    color: #323130;
}
`;

    if (!content.includes('Windows 11 Styling')) {
        content += windows11Styles;
        fs.writeFileSync(filePath, content);
        console.log(`Updated CSS file: ${filePath}`);
    }
}

// Function to update UI-specific files
function updateUi(filePath) {
    let content = fs.readFileSync(filePath, 'utf-8');

    // Example: Modify XML-like UI structure
    content = content.replace(/theme=".*?"/, 'theme="Windows 11"');
    content = content.replace(/borderRadius=".*?"/, 'borderRadius="8px"');

    fs.writeFileSync(filePath, content);
    console.log(`Updated UI file: ${filePath}`);
}

// Main function
function recodeUi(filePath, theme) {
    const ext = path.extname(filePath).toLowerCase();
    try {
        if (ext === '.json') {
            updateJson(filePath);
        } else if (ext === '.css') {
            updateCss(filePath);
        } else if (ext === '.ui') {
            updateUi(filePath);
        } else {
            console.log(`Unsupported file type: ${filePath}`);
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
