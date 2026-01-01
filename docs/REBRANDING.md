# Rebranding Complete: ZorroATI ? ZorroBridge

## Changes Made

### 1. AddOn Renamed
- **Old:** `ninjatrader-addon/ZorroATI.cs`
- **New:** `ninjatrader-addon/ZorroBridge.cs`

**Class name changed:**
```csharp
// Old
public class ZorroATI : AddOnBase

// New
public class ZorroBridge : AddOnBase
```

**Branding updated:**
```csharp
Name = "Zorro Bridge"  // was "Zorro ATI Bridge"
Print("[Zorro Bridge] ...")  // was "[Zorro ATI] ..."
```

---

### 2. Repository URL Updated
- **Old:** `https://github.com/bigale/ninjatrader-ati-zorro-plugin.git`
- **New:** `https://github.com/bigale/ninjatrader-zorro-plugin.git`

All documentation now references the new URL.

---

### 3. Files Updated

| File | Changes |
|------|---------|
| `ninjatrader-addon/ZorroBridge.cs` | Renamed from ZorroATI.cs, class renamed |
| `README.md` | Repository URL, AddOn name |
| `CHANGELOG.md` | Repository URL, AddOn name |
| `CONTRIBUTING.md` | Repository URL |
| `docs/INSTALLATION.md` | AddOn name, repository URL |
| `docs/ARCHITECTURE.md` | AddOn name, repository URL |
| `docs/TROUBLESHOOTING.md` | AddOn name references |
| `src/NT8Plugin.cpp` | Comment references |
| `scripts/README.md` | AddOn name |

---

## Action Required

### For New Installations

**No changes needed!** Just follow the installation guide as-is:

1. Copy `ZorroBridge.cs` to AddOns folder
2. Compile in NinjaTrader (F5)
3. Look for `[Zorro Bridge] Zorro Bridge listening on port 8888` in Output

---

### For Existing Installations

**?? Important:** You need to remove the old AddOn and install the new one.

#### Step 1: Remove Old AddOn

1. **Delete old file:**
   ```
   Delete: Documents\NinjaTrader 8\bin\Custom\AddOns\ZorroATI.cs
   ```

2. **Recompile NinjaTrader:**
   - Press **F5** in NinjaTrader
   - This removes the old AddOn from memory

#### Step 2: Install New AddOn

1. **Copy new file:**
   ```
   Copy: ninjatrader-addon\ZorroBridge.cs
   To: Documents\NinjaTrader 8\bin\Custom\AddOns\
   ```

2. **Compile:**
   - Press **F5** in NinjaTrader

3. **Verify:**
   - Look for `[Zorro Bridge] Zorro Bridge listening on port 8888` in Output
   - Should see this on startup

---

## Why the Change?

### Original Name: "ZorroATI"
- **Confusing:** "ATI" stands for "Automated Trading Interface"
- **Problem:** Implied we were using NinjaTrader's old ATI (NtDirect.dll)
- **Reality:** We use a modern TCP bridge, **not** the old ATI DLL

### New Name: "ZorroBridge"
- **Clear:** Indicates it's a bridge between Zorro and NinjaTrader
- **Accurate:** Reflects TCP bridge architecture
- **Professional:** No confusion with deprecated technologies

---

## Repository Name Change

### Old
```
github.com/bigale/ninjatrader-ati-zorro-plugin
```
**Issues:**
- "ati" implies old technology
- Unnecessarily long

### New
```
github.com/bigale/ninjatrader-zorro-plugin
```
**Benefits:**
- Clean, simple name
- No misleading technology references
- Easier to remember and share

---

## Compatibility

### ? Everything Still Works
- **Plugin:** NT8.dll unchanged (still works)
- **Architecture:** TCP bridge unchanged
- **Port:** localhost:8888 unchanged
- **Protocol:** All commands unchanged
- **Functionality:** 100% identical

### ?? Only Name Changed
- File name: `ZorroATI.cs` ? `ZorroBridge.cs`
- Class name: `ZorroATI` ? `ZorroBridge`
- Branding in Output window
- Documentation references

---

## Testing After Upgrade

After installing ZorroBridge, verify it works:

1. **Check NinjaTrader Output:**
   ```
   [Zorro Bridge] Zorro Bridge listening on port 8888
   ```

2. **Run test script in Zorro:**
   ```c
   // NT8Test.c should still work exactly the same
   ```

3. **Expected behavior:**
   - Connection works ?
   - Market data works ?
   - Orders work ?
   - All functionality identical ?

---

## Summary

| Aspect | Old | New |
|--------|-----|-----|
| **AddOn File** | ZorroATI.cs | ZorroBridge.cs |
| **Class Name** | ZorroATI | ZorroBridge |
| **Display Name** | Zorro ATI Bridge | Zorro Bridge |
| **Repository** | ninjatrader-ati-zorro-plugin | ninjatrader-zorro-plugin |
| **Functionality** | ? Works | ? Works (identical!) |

---

## Git Commit

```
Commit: 18b1421
Message: Rename AddOn to ZorroBridge and update repository URL
Date: 2025-01-01
```

**Changes:**
- Renamed ZorroATI.cs ? ZorroBridge.cs
- Updated all documentation
- Changed repository URL
- Updated all branding

---

## Need Help?

If you encounter any issues after the rename:

1. **Check Output window** - Should see "Zorro Bridge" messages
2. **Verify file copied** - Check AddOns folder has ZorroBridge.cs
3. **Recompile** - Press F5 in NinjaTrader
4. **Check logs** - Look for connection messages

Everything should work exactly as before, just with clearer naming!
