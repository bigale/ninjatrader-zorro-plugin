# Copilot Instructions for NinjaTrader Zorro Plugin

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
- **Live-only trading**: No historical data support

### Key Components
1. **NT8.dll** - C++ broker plugin (32-bit)
2. **ZorroBridge.cs** - NinjaScript AddOn (C#)
3. **TcpBridge** - Localhost TCP communication layer

### Common Terms
- Use "ZorroBridge" (not "ZorroATI")
- Use "AddOn" (not "plugin" for C# component)
- Use "plugin" for NT8.dll specifically
- Clarify "live trading" vs "playback/simulation"

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
- PowerShell/batch scripts in `private-docs/` use local `zorro/` folder
- All paths point to repo structure, no external dependencies

### Running Tests
```bash
# From repository root
.\private-docs\test-zorro.ps1      # Basic tests
.\private-docs\test-full.ps1       # Full test suite
.\private-docs\build-and-test.bat  # Build + test
