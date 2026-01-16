# Copilot Instructions for NinjaTrader Zorro Plugin

## Quick Reference Paths

### Key File Locations
```
Plugin DLL:     C:\Zorro\Plugin\NT8.dll
AddOn Source:   C:\Users\zorro\Documents\NinjaTrader 8\bin\Custom\AddOns\ZorroBridge.cs
Assets File:    C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\Log\Assets.csv
AssetsRithmic:  C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\Log\AssetsRithmic.csv
Accounts File:  C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\History\Accounts.csv
Test Log:       C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\Log\AutoTradeTest_demo.log
```

### Build & Deploy Workflow

**Quick rebuild with debug:**
```bash
# From repo root
cd build
cmake --build . --config Release
Copy-Item "Release\NT8.dll" "C:\Zorro\Plugin\NT8.dll" -Force
```

**Update AddOn:**
```bash
Copy-Item "ninjatrader-addon\ZorroBridge.cs" "C:\Users\zorro\Documents\NinjaTrader 8\bin\Custom\AddOns\" -Force
# Then press F5 in NinjaTrader to recompile
```

**Or use convenience script:**
```bash
.\rebuild-debug.bat
```

### Testing with TradeTest.c

1. **Start NinjaTrader** (must be first)
   - Verify Output shows: `[Zorro Bridge] Zorro Bridge listening on port 8888`
   - Clear Output window (right-click ? Clear)

2. **Start Zorro**
   - Select NT8-Sim account
   - Load TradeTest script (from `scripts/`)
   - Click Trade

3. **Watch BOTH windows:**
   - **Zorro Console** - Shows plugin-side debug (TcpBridge)
   - **NT Output** - Shows AddOn-side debug (ZorroBridge)

4. **Test order:**
   - Click "Buy Long" (default is MKT Order)
   - Observe debug output in both windows

### Debug Output to Capture

**From Zorro Console:**
```
[TcpBridge] Sending order command: PLACEORDER:BUY:MES 03-26:1:MARKET:0.00
[TcpBridge] Response: ORDER:12345
```

**From NT Output:**
```
[Zorro Bridge] ==== HandlePlaceOrder START ====
[Zorro Bridge] Parsed order: Action: BUY, Quantity: 1, OrderType: MARKET
[Zorro Bridge] Instrument found: MES 03-26
[Zorro Bridge] Order created: NT123456789
[Zorro Bridge] ==== HandlePlaceOrder SUCCESS ====
```

### Checking Zorro Logs

**AutoTradeTest log file:**
```
C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\Log\AutoTradeTest_demo.log
```

**View recent log entries:**
```powershell
Get-Content "C:\Users\zorro\source\repos\ninjatrader-zorro-plugin\zorro\Log\AutoTradeTest_demo.log" -Tail 50
```

### Configuration Files

**Assets.csv** - Symbol must match between Name and usage:
```csv
MESH26,6869.00,0.250000,0.0,0.0,0.25,1.25,500,CME,5,0.52,MES 03-26
```
- Name: `MESH26` (what Zorro uses)
- Symbol: `MES 03-26` (what NT uses)
- PIP: `0.25` (tick size)
- PIPCost: `1.25` (tick value)


## Markdown File Creation Rules

### Critical: Prevent Unicode Display Issues

**ALWAYS use these characters in markdown:**

? **Correct:**
- Checkmarks: `?` `?`
- Arrows: `?` `?` `?` `?`
- Boxes: Use Mermaid diagrams instead of ASCII art
- Bullets: Standard `-` or `*`

? **NEVER use:**
- Unicode box-drawing characters: `?` `?` `?` `?` `?` `?` (causes `?` on some systems)
- Fancy Unicode symbols that may not render in all environments
- Non-standard quotation marks: `"` `"` (use `"` instead)

### Mermaid Diagram Rules

**When creating Mermaid diagrams, ALWAYS specify text color:**

```markdown
```mermaid
graph LR
    A[Node Text]
    
    style A fill:#e1f5ff,stroke:#333,stroke-width:2px,color:#000
    //                                              ^^^^^^^^^^^
    //                                              REQUIRED: Black text
```

**Template for styled nodes:**
```
style NodeName fill:#HEXCOLOR,stroke:#333,stroke-width:2px,color:#000
```

### File Encoding

- **Always use UTF-8 encoding**
- Never use ANSI or Windows-1252
- Verify encoding when creating files

### Best Practices

1. **Test rendering** - Preview markdown before committing
2. **Use standard ASCII** - When in doubt, use plain ASCII characters
3. **Prefer Mermaid** - For diagrams, use Mermaid over ASCII art
4. **Consistent styling** - All diagram nodes need `color:#000` for readability

## Code Style

### C++ Files
- Use C++17 standard features
- Follow existing code formatting
- Add comments for complex logic only
- Keep functions focused and small

### C# Files (NinjaScript AddOn)
- Follow Microsoft naming conventions
- Use XML comments for public methods
- Keep consistent with existing AddOn code

### Lite-C Files (Zorro Scripts)
- Declare ALL variables at function start
- No mid-function variable declarations
- Use global variables when needed
- Keep scripts simple and readable

## Documentation Structure

### Public Documentation (`docs/`)
- User-facing content only
- Clear, concise language
- Include code examples
- Link to related docs

### Development Documentation (`private-docs/`)
- Technical details
- Build notes
- Architecture discussions
- Migration guides

## Project-Specific Notes

### Architecture
- **TCP Bridge**: Plugin communicates via localhost:8888
- **No ATI/NtDirect.dll**: Modern NT8 8.1+ compatible only
- **32-bit Plugin**: Required for Zorro compatibility

### Key Components
1. **NT8.dll** - C++ broker plugin (32-bit)
2. **ZorroBridge.cs** - NinjaScript AddOn (C#)
3. **TcpBridge** - Localhost TCP communication layer

### Common Terms
- Use "ZorroBridge" (not "ZorroATI")
- Use "AddOn" (not "plugin" for C# component)
- Use "plugin" for NT8.dll specifically

## Testing Workflow

### Local Zorro Installation
- **Location:** `zorro/` folder in repository root (gitignored)
- **Strategy Folder:** Configured to point to `scripts/` in repo
- **DO NOT copy scripts** - They're loaded directly from `scripts/` folder
- **Test scripts ARE tracked** in Git (in `scripts/` folder)

### Testing Scripts
- Test scripts in `scripts/` folder:
  - `NT8Test.c` - Connection and data tests
  - `AutoTradeTest.c` - Automated trading tests
  - `SimpleNT8Test.c` - Quick smoke test
  - `TradeTest.c` - Manual order testing (works on holidays!)
- PowerShell/batch scripts in `private-docs/` use local `zorro/` folder
- All paths point to repo structure, no external dependencies

### Running Tests
```bash
# From repository root
.\private-docs\test-zorro.ps1      # Basic tests
.\private-docs\test-full.ps1       # Full test suite
.\private-docs\build-and-test.bat  # Build + test
```

### **CRITICAL: Test Before Commit**
?? **NEVER commit code changes without confirming tests pass!**

**Required workflow:**
1. Make code changes
2. Build the plugin: `.\rebuild-debug.bat` OR manual build
3. Update AddOn if changed (copy + F5 in NT)
4. Run tests: TradeTest.c for order testing
5. **WAIT for user confirmation** that tests pass
6. Only then commit and push

**Why this matters:**
- Zorro scripts may have compilation errors
- Tests may fail due to runtime issues
- User needs to observe test results
- Broken commits waste time and create bad history

**If tests fail:**
- Check debug output in BOTH Zorro console and NT Output
- Fix the issue
- Repeat steps 2-4
- Get user confirmation before committing

## Common Debugging Scenarios

### Orders Not Filling
1. Check symbol matches: Assets.csv Name vs TradeTest ASSET define
2. Verify Account.csv has correct Plugin (NT8.dll) and Assets reference
3. Review debug output:
   - Zorro: `[TcpBridge] Sending order command:`
   - NT: `[Zorro Bridge] ==== HandlePlaceOrder START ====`
4. Check NT Orders tab - does order appear?
5. Look for exceptions in NT Output window

### Connection Issues
1. NT Output must show: `[Zorro Bridge] Zorro Bridge listening on port 8888`
2. Zorro must show: `# NT8 connected to account: Sim101`
3. Port 8888 must be free (no other apps using it)
4. Restart both NT and Zorro if connection lost

### Market Data Issues
1. Verify NT is connected to data feed
2. Check symbol exists in NT (open a chart)
3. Look for subscription messages in NT Output
4. Asset Name in Assets.csv must match script usage

## Git Commit Messages in PowerShell

**NEVER use multi-line strings directly in git commit -m**

PowerShell has issues with newlines in command arguments. Use one of these methods:

### Method 1: Commit Message File (RECOMMENDED)
```powershell
# Create temp file with full message
@"
Add configurable logging system

Major improvements:
- Added LogLevel enum (TRACE/DEBUG/INFO/WARN/ERROR)
- Heartbeat logs status every 10 seconds
- SETLOGLEVEL command for runtime changes

Benefits:
- Production: Clean INFO logs
- Debugging: Switch to DEBUG/TRACE without restart
- Performance: Reduced console spam
"@ | Out-File -FilePath .git/COMMIT_EDITMSG_temp -Encoding UTF8

# Commit using file
git commit -F .git/COMMIT_EDITMSG_temp
```

### Method 2: Simple One-Line (for quick commits)
```powershell
git commit -m "Add feature X - description here"
```

### Method 3: Multiple -m Flags (for paragraphs)
```powershell
git commit -m "Title line" `
  -m "First paragraph of details" `
  -m "Second paragraph with more info"
```

**RULE:** Always use Method 1 for detailed commit messages to avoid PowerShell newline issues.
