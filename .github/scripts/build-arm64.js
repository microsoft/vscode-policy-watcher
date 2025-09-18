#!/usr/bin/env node

/**
 * LEGACY: Local ARM64 cross-compilation using Docker + QEMU
 * 
 * ⚠️  THIS SCRIPT IS NO LONGER NEEDED FOR REGULAR DEVELOPMENT ⚠️
 * 
 * As of January 2025, GitHub Actions provides native ARM64 runners.
 * This script is kept only for special cases:
 * - Testing on systems without native ARM64 support
 * - Development on non-GitHub CI systems
 * - Local debugging of ARM64-specific issues
 * 
 * For normal development, use:
 * - npm run prebuild (builds for current platform)
 * - GitHub Actions (handles all platforms automatically)
 */

console.log('⚠️  WARNING: This script is legacy and rarely needed.');
console.log('💡 Consider using GitHub Actions with native ARM64 runners instead.');
console.log('🚀 For local development, use: npm run prebuild');
console.log('');
console.log('Continue anyway? (y/N)');

process.stdin.once('data', (data) => {
  if (data.toString().trim().toLowerCase() !== 'y') {
    console.log('Aborted. Use GitHub Actions or npm run prebuild instead.');
    process.exit(0);
  }
  main();
});

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
  
  console.log('🔧 Setting up ARM64 cross-compilation test...');
  
  // Create Dockerfile for ARM64 build
  const dockerfile = `
FROM --platform=linux/arm64 node:20-bullseye

WORKDIR /usr/src/build

# Install build dependencies
RUN apt-get update && apt-get install -y python3 python3-pip build-essential git && \\
    pip3 install setuptools

COPY package*.json ./
COPY binding.gyp ./
COPY src/ ./src/
COPY index.js ./
COPY index.d.ts ./

RUN npm ci --ignore-scripts
RUN npm run prebuild
RUN npm test
`;

  fs.writeFileSync(path.join(rootDir, 'Dockerfile.arm64'), dockerfile);

  console.log('🐳 Building ARM64 binary using Docker...');
  
  // Set up QEMU and buildx if not already done
  try {
    run('docker buildx ls');
  } catch {
    console.log('Setting up Docker Buildx...');
    run('docker buildx create --name arm64-builder --use --bootstrap');
  }

  // Build ARM64 binary
  run(`docker buildx build \\
    --platform linux/arm64 \\
    --output type=local,dest=./output-arm64 \\
    --file Dockerfile.arm64 \\
    --no-cache \\
    .`);

  // Copy prebuilds
  const outputDir = path.join(rootDir, 'output-arm64');
  const prebuildsDir = path.join(rootDir, 'prebuilds');
  
  if (!fs.existsSync(prebuildsDir)) {
    fs.mkdirSync(prebuildsDir);
  }

  const sourcePrebuilds = path.join(outputDir, 'usr', 'src', 'build', 'prebuilds');
  if (fs.existsSync(sourcePrebuilds)) {
    const files = fs.readdirSync(sourcePrebuilds);
    files.forEach(file => {
      if (file.endsWith('.tar.gz')) {
        fs.copyFileSync(
          path.join(sourcePrebuilds, file),
          path.join(prebuildsDir, file)
        );
        console.log(`✅ Copied ARM64 prebuild: ${file}`);
      }
    });
  }

  // Cleanup
  fs.rmSync(path.join(rootDir, 'Dockerfile.arm64'), { force: true });
  fs.rmSync(outputDir, { recursive: true, force: true });

  console.log('\n🎉 ARM64 cross-compilation completed successfully!');
  console.log('\nGenerated prebuilds:');
  if (fs.existsSync(prebuildsDir)) {
    const files = fs.readdirSync(prebuildsDir);
    files.forEach(file => console.log(`  - ${file}`));
  }
}

if (require.main === module) {
  main();
}

module.exports = { main };