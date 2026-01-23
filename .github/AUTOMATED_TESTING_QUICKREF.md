# Automated Testing - Copilot Quick Reference

## ?? Quick Start - Automated Testing

**This is the primary method for testing - always use this first!**

```powershell
# Run automated test (does everything automatically)
.\automated-zorro-test.ps1 -DeleteCache

# With log viewing after completion
.\automated-zorro-test.ps1 -DeleteCache -OpenLogs
```

## What the Automated Test Does

1. ? Deletes old logs (diag.txt, test logs, debug logs)
2. ? Deletes compiled script cache (.zc files) - **critical for recompilation**
3. ? Checks if NinjaTrader is running
4. ? Runs: `zorro.exe -diag -trade DirectBuyTest` (LIVE mode)
5. ? Monitors process (30s timeout)
6. ? **Analyzes output automatically**
7. ? Reports: "BrokerBuy2 called?" (SUCCESS/FAILURE)
8. ? Shows next steps if failed

## Expected Output

### ? Success
```
[PASS] BrokerLogin called
[PASS] BrokerAsset called
[PASS] BrokerBuy2 called  ? The goal!

[SUCCESS] BrokerBuy2 was called!
  The plugin function executed successfully.
```

### ? Failure
```
[FAIL] BrokerBuy2 NOT called  ? Script quit early
[FAIL] DEBUG output NOT found  ? Script didn't recompile

POSSIBLE CAUSES:
  1. Script did not recompile
  2. .trd file replay
  3. Global variable persistence

NEXT STEPS:
  1. Delete compiled cache and retry
```

## Common Issues Automatically Fixed

| Issue | Manual Detection | Automated Script |
|-------|------------------|------------------|
| Script didn't recompile | Check .log manually | `-DeleteCache` flag |
| Process hung | Manual timeout | Auto 30s timeout |
| Log analysis | Manual search | Auto PASS/FAIL |
| Missing BrokerBuy2 | Search diag.txt | Highlighted in summary |

## File Locations

- **Script:** `automated-zorro-test.ps1`
- **Docs:** `docs\AUTOMATED_TESTING.md`
- **Output:** `zorro\diag.txt` + `zorro\Log\DirectBuyTest_*.log`

## Integration with Development Workflow

```powershell
# 1. Make code changes to plugin/addon/script
# 2. Rebuild if needed
.\rebuild-debug.bat

# 3. Run automated test
.\automated-zorro-test.ps1 -DeleteCache

# 4. If SUCCESS ? commit
# 5. If FAILURE ? check output, fix, repeat
```

## Parameters

```powershell
-DeleteCache       # Delete .zc compiled cache (recommended)
-OpenLogs          # Open log files in notepad after test
-TimeoutSeconds 60 # Custom timeout (default 30)
```

## Exit Codes

- `0` = Success (BrokerBuy2 called)
- `1` = Failure (BrokerBuy2 NOT called)

Use in scripts:
```powershell
.\automated-zorro-test.ps1 -DeleteCache
if ($LASTEXITCODE -eq 0) {
    Write-Host "Ready to commit!"
}
```

---

**ALWAYS use automated testing before committing code!**
