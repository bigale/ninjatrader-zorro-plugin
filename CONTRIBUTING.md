# Contributing to NinjaTrader 8 Zorro Plugin

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and constructive
- Focus on the code, not the person
- Help create a welcoming environment for all

## How to Contribute

### Reporting Bugs

Before creating bug reports:
1. Check existing issues
2. Test with latest version
3. Verify it's not a configuration issue

**Include in bug report:**
- Zorro version
- NinjaTrader version
- Plugin version
- Steps to reproduce
- Expected vs actual behavior
- Relevant log excerpts
- Test script (if applicable)

### Suggesting Enhancements

Feature requests are welcome! Please:
1. Check if already suggested
2. Explain the use case
3. Describe the proposed solution
4. Consider backward compatibility

### Pull Requests

1. **Fork the repository**
2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow existing code style
   - Add comments for complex logic
   - Update documentation if needed

4. **Test thoroughly**
   - Test with simulation account
   - Verify no regressions
   - Test edge cases

5. **Commit with clear messages**
   ```bash
   git commit -m "Add stop-loss order support"
   ```

6. **Push and create PR**
   ```bash
   git push origin feature/your-feature-name
   ```

## Development Setup

### Prerequisites
- Visual Studio 2019+ with C++ workload
- CMake 3.15+
- NinjaTrader 8.1+
- Zorro 2.62+

### Build Process
```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A Win32 ..
cmake --build . --config Release
```

### Testing

**Manual Testing:**
1. Copy NT8.dll to Zorro Plugin folder
2. Install AddOn in NinjaTrader
3. Run NT8Test.c script
4. Verify all tests pass

**Test Checklist:**
- [ ] Plugin loads in Zorro
- [ ] Connection to NinjaTrader works
- [ ] Account info retrieves correctly
- [ ] Market data subscribes
- [ ] Orders place successfully
- [ ] Positions track correctly
- [ ] Errors handle gracefully

## Code Style

### C++ (Plugin)

```cpp
// Use descriptive names
static bool g_connected = false;

// Comment complex logic
// Convert Zorro DATE to Unix timestamp
__time64_t ConvertDATEToUnix(DATE date) {
    return (__time64_t)((date - 25569.0) * 24.0 * 60.0 * 60.0);
}

// Error handling
if (!g_bridge || !g_connected) {
    LogError("Not connected");
    return 0;
}

// Consistent formatting
if (condition) {
    DoSomething();
} else {
    DoSomethingElse();
}
```

### C# (AddOn)

```csharp
// Use Microsoft naming conventions
private Account currentAccount;

// Add XML comments for public methods
/// <summary>
/// Handles the SUBSCRIBE command from Zorro
/// </summary>
private string HandleSubscribe(string[] parts)
{
    // Implementation
}

// Log important events
Print($"[Zorro ATI] Subscribed to {instrumentName}");
```

## File Structure

```
ninjatrader-zorro-plugin/
??? include/          # Header files
??? src/              # Source files
??? ninjatrader-addon/ # NinjaScript AddOn
??? docs/             # Public documentation
??? private-docs/     # Development notes
??? build/            # Build output (gitignored)
??? CMakeLists.txt    # Build configuration
```

## Documentation

### Code Comments

- Comment **why**, not **what**
- Explain complex algorithms
- Document assumptions
- Note platform-specific code

### User Documentation

Update relevant docs when changing:
- API behavior
- Configuration requirements
- Supported features
- Error messages

**Documentation files:**
- `README.md` - Project overview
- `docs/INSTALLATION.md` - Setup instructions
- `docs/API_REFERENCE.md` - API details
- `docs/TROUBLESHOOTING.md` - Common issues
- `CHANGELOG.md` - Version history

## Testing Guidelines

### Simulation Testing

**Always test with simulation accounts first!**

```c
// Test script template
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    // Your test code
    
    quit("Test complete");
}
```

### Edge Cases to Test

- [ ] Market closed
- [ ] No data feed
- [ ] Invalid symbols
- [ ] Insufficient margin
- [ ] Network disconnection
- [ ] Rapid order placement
- [ ] Position flipping (long ? short)
- [ ] Partial fills

## Commit Message Guidelines

### Format

```
type(scope): subject

body (optional)

footer (optional)
```

### Types

- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation only
- **style**: Formatting, no code change
- **refactor**: Code restructuring
- **test**: Adding tests
- **chore**: Maintenance

### Examples

```
feat(orders): add stop-loss order support

Implements stop orders via NinjaTrader API.
- Added StopDist parameter handling
- Updated order placement logic
- Added tests for stop orders

Closes #42
```

```
fix(symbols): correct month code conversion

Fixed issue where month code 'U' (September) was
incorrectly converted to month 08 instead of 09.
```

## Release Process

1. Update `CHANGELOG.md`
2. Increment version number
3. Test all features
4. Create release tag
5. Build release binaries
6. Update documentation
7. Create GitHub release

## Feature Requests

### High Priority
- Stop-loss orders
- Take-profit orders
- Better error handling

### Medium Priority
- Historical data support
- Multi-account support
- Order modification

### Low Priority
- Performance optimizations
- Additional order types
- UI improvements

## Questions?

- Open an issue for questions
- Check existing documentation first
- Be specific in your questions

---

Thank you for contributing to the project! ??
