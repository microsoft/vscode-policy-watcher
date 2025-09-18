#!/usr/bin/env node

/**
 * Build script for vscode-policy-watcher
 * Builds prebuilt binaries for the current platform
 */

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

function run(command, options = {}) {
  console.log(`+ ${command}`);
  try {
    execSync(command, { 
      stdio: 'inherit', 
      cwd: path.resolve(__dirname, '..', '..'),
      ...options 
    });
  } catch (error) {
    console.error(`Failed to run: ${command}`);
    process.exit(1);
  }
}

function main() {
  const rootDir = path.resolve(__dirname, '..', '..');
  
  // Clean build and prebuilds directories
  console.log('Cleaning build directories...');
  const buildDir = path.join(rootDir, 'build');
  const prebuildsDir = path.join(rootDir, 'prebuilds');
  
  if (fs.existsSync(buildDir)) {
    fs.rmSync(buildDir, { recursive: true, force: true });
  }
  
  if (fs.existsSync(prebuildsDir)) {
    fs.rmSync(prebuildsDir, { recursive: true, force: true });
  }

  // Install dependencies (ignore scripts to avoid auto-rebuild)
  console.log('Installing dependencies...');
  run('npm install --ignore-scripts');

  // Create prebuild
  console.log('Creating prebuild...');
  run('npm run prebuild');

  // Test the build
  console.log('Testing the build...');
  run('npm test');

  console.log('\n✅ Build completed successfully!');
  
  // List generated prebuilds
  if (fs.existsSync(prebuildsDir)) {
    console.log('\nGenerated prebuilds:');
    const files = fs.readdirSync(prebuildsDir);
    files.forEach(file => console.log(`  - ${file}`));
  }
}

if (require.main === module) {
  main();
}

module.exports = { run, main };