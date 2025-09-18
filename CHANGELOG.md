# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- GitHub Actions CI/CD pipeline for automated building and testing
- Support for prebuilt binaries across multiple platforms (Windows x64/x86, macOS x64/arm64, Linux x64/arm64)
- Automated release process with GitHub Actions
- Build scripts for local development
- **Native ARM64 support** using GitHub's new ARM64 hosted runners (January 2025)

### Changed
- Migrated from Azure Pipelines to GitHub Actions
- Updated package.json to support prebuild system
- Enhanced README with installation and development instructions
- **Upgraded from QEMU emulation to native ARM64 runners** for 3-4x faster builds

### Technical
- Added prebuild and prebuild-install dependencies
- Added build and release automation scripts
- Added comprehensive CI testing matrix
- **Implemented native ARM64 builds** using `ubuntu-22.04-arm` runners
- Kept legacy QEMU builds as optional fallback for additional testing

### Performance
- **ARM64 build time reduced from ~8-10 minutes to ~2-3 minutes**
- Simplified ARM64 build configuration (no Docker/QEMU setup required)
- Improved reliability with native ARM64 environment

## [1.3.2] - Previous Release

### Initial Features
- Policy watching functionality for VS Code
- Support for Windows, macOS, and Linux platforms
- Node.js addon implementation using node-addon-api