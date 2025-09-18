# GitHub Actions ARM64 Support Update

## 🎉 Major Update: Native ARM64 Runners Available!

As of **January 16, 2025**, GitHub Actions now provides **native Linux ARM64 hosted runners** for public repositories (free, in public preview).

## 📋 What Changed

### ✅ **Before (QEMU Emulation)**
```yaml
# Slow QEMU-based cross-compilation
prebuild-linux-arm:
  runs-on: ubuntu-latest  # x64 host
  steps:
    - uses: docker/setup-qemu-action@v3  # Emulator setup
    - run: docker buildx build --platform linux/arm64  # 3-5x slower
```

### 🚀 **Now (Native ARM64)**
```yaml
# Fast native ARM64 compilation  
prebuild:
  strategy:
    matrix:
      include:
        - os: ubuntu-22.04-arm  # Native ARM64 runner
          arch: arm64
```

## 🏃‍♂️ **Available ARM64 Runners**

GitHub now provides these native ARM64 runners for **public repositories**:

- `ubuntu-24.04-arm` - Ubuntu 24.04 on ARM64
- `ubuntu-22.04-arm` - Ubuntu 22.04 on ARM64

**Requirements:**
- ✅ **Public repositories only** (free)
- ❌ Not available for private repositories yet
- ⚡ Up to **40% performance boost** vs previous generation
- 🔧 **4 vCPU** Cobalt 100-based processors

## 📊 **Performance Comparison**

| Method | Build Time | Setup Complexity | Performance |
|--------|------------|------------------|-------------|
| **QEMU Emulation** | ~8-10 min | Medium | 100% (baseline) |
| **Native ARM64** | ~2-3 min | Simple | **300-400%** faster |

## 🔧 **Updated Configuration**

Our `cls-policy-watcher` project now uses:

### **Simplified Build Matrix**
```yaml
strategy:
  matrix:
    include:
      # Standard builds
      - { os: ubuntu-20.04, node: 20.x, arch: x64 }
      - { os: macos-latest, node: 20.x, arch: x64 }
      - { os: macos-latest, node: 20.x, arch: arm64 }
      - { os: windows-latest, node: 20.x, arch: x64 }
      - { os: windows-latest, node: 20.x, arch: arm64 }
      
      # Native ARM64 builds
      - { os: ubuntu-22.04-arm, arch: arm64 }
```

### **Requirements**
- **Node.js**: 20.0.0+ (18.x support removed)
- **Platforms**: Windows x64/arm64, macOS x64/arm64, Linux x64/arm64

## 🎯 **Benefits**

1. **⚡ Faster Builds**: 3-4x faster than QEMU emulation
2. **🔧 Simpler Config**: No Docker/QEMU setup needed
3. **💰 Cost Effective**: Still free for public repos
4. **🎯 Native Performance**: True ARM64 environment
5. **🧪 Better Testing**: Can run tests natively

## 📚 **References**

- [GitHub Blog: ARM64 Runners Announcement](https://github.blog/changelog/2025-01-16-linux-arm64-hosted-runners-now-available-for-free-in-public-repositories-public-preview/)
- [GitHub Docs: ARM64 Runners](https://docs.github.com/en/actions/using-github-hosted-runners/using-github-hosted-runners/about-github-hosted-runners#supported-runners-and-hardware-resources)
- [Community Discussion](https://github.com/orgs/community/discussions/148648)

## 🚀 **Migration Notes**

1. **Immediate**: All new builds use native ARM64 runners
2. **Backward Compatible**: QEMU builds available as fallback
3. **Performance**: Expect significantly faster CI times
4. **Queue Times**: May be longer during peak usage (preview period)

This represents a major improvement in our CI/CD pipeline efficiency! 🎉