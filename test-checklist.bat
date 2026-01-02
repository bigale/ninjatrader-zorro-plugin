@echo off
REM Quick checklist display for testing
echo ========================================
echo TESTING CHECKLIST
echo ========================================
echo.
echo [ ] 1. NinjaTrader is running
echo [ ] 2. Press F5 in NT to recompile AddOn
echo [ ] 3. NT Output shows: "listening on port 8888"
echo [ ] 4. Clear NT Output window
echo [ ] 5. Start Zorro
echo [ ] 6. Select NT8-Sim account
echo [ ] 7. Load TradeTest script
echo [ ] 8. Click Trade
echo [ ] 9. Click "Buy Long"
echo [ ] 10. Capture debug output from BOTH windows
echo.
echo ========================================
echo.
echo Files already copied:
echo   - Plugin DLL to C:\Zorro\Plugin\NT8.dll
echo   - AddOn to NT8 AddOns folder
echo.
echo Ready to test!
echo.
pause
