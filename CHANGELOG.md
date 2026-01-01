# Changelog

All notable changes to the NinjaTrader 8 Zorro Plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-01

### Added
- Initial release of NT8 Zorro Plugin
- TCP bridge architecture for NT8 8.1+ compatibility
- NinjaScript AddOn (ZorroBridge.cs) for NinjaTrader side
- Full broker API implementation:
  - BrokerOpen - Plugin initialization
  - BrokerLogin - TCP connection to NinjaTrader
  - BrokerTime - Keep-alive and time synchronization
  - BrokerAsset - Market data subscription and price retrieval
  - BrokerAccount - Account balance, margin, P&L
  - BrokerBuy2 - Order placement (market & limit)
  - BrokerSell2 - Position closing
  - BrokerTrade - Order status queries
  - BrokerCommand - Extended commands
- Symbol format conversion:
  - MESH26 ? MES 03-26
  - MES 0326 ? MES 03-26
  - Month code support (H=Mar, M=Jun, U=Sep, Z=Dec)
- Broker commands:
  - GET_POSITION - Query net position
  - GET_AVGENTRY - Average entry price
  - GET_COMPLIANCE - NFA mode
  - GET_BROKERZONE - Timezone info
  - GET_MAXTICKS - Historical data availability
  - GET_WAIT - Polling interval
  - SET_ORDERTYPE - Order time-in-force
  - SET_DIAGNOSTICS - Debug logging
  - DO_CANCEL - Order cancellation
- Real-time market data streaming
- Position tracking with average entry
- CMake build system with 32-bit support
- Comprehensive documentation:
  - README.md
  - Installation guide
  - Getting started guide
  - API reference
  - Troubleshooting guide
- Example test scripts:
  - NT8Test.c - Comprehensive feature test
  - SimpleNT8Test.c - Basic connectivity test
- Build automation scripts:
  - build-and-install.bat
  - build.bat
  - install.bat

### Technical Details
- Language: C++17
- Build: CMake 3.15+, Visual Studio 2019+
- Architecture: 32-bit (x86) for Zorro compatibility
- Communication: TCP localhost:8888
- NinjaScript: C# AddOn for NT8 8.1+

### Known Limitations
- Live trading only (no historical data)
- One account at a time
- Contract specifications must be in Zorro asset file
- Stop orders not yet implemented (planned v1.1)
- OCO/Bracket orders not yet implemented (planned v2.0)

---

## [Unreleased]

### Planned for v1.1
- Stop-loss order support
- Take-profit order support
- Order modification capability
- Enhanced error handling and reconnection logic
- Order state machine for partial fills
- Real-time order update notifications

### Planned for v2.0
- Historical data support via separate mechanism
- Multi-account support
- OCO (One-Cancels-Other) orders
- Bracket orders (entry + stop + target)
- Advanced order types
- Performance optimizations

---

## Development History

### 2025-01-01
- ? Project completed and tested
- ? All core features operational
- ? Documentation complete
- ? Ready for production use

### 2024-12-31
- ? Symbol conversion working (MESH26 ? MES 03-26)
- ? AddOn null-checking for market data
- ? Live-only trading confirmed functional
- ? Comprehensive test suite created

### 2024-12-30
- ? TCP bridge architecture implemented
- ? NinjaScript AddOn created for NT8 8.1+
- ? Replaced legacy NtDirect.dll approach
- ? CMake build system configured

### Pre-2024-12-30
- Initial proof of concept
- Research and architecture planning

---

## Migration Notes

### From NtDirect.dll Approach
If upgrading from an older version that used NtDirect.dll:

**What Changed:**
- Removed dependency on NtDirect.dll
- Added TCP bridge architecture
- Added NinjaScript AddOn requirement
- Improved NT8 8.1+ compatibility

**Migration Steps:**
1. Remove old NT8.dll from Plugin folder
2. Build new version from source
3. Install ZorroBridge.cs AddOn in NinjaTrader
4. Update accounts.csv if needed
5. Test with simulation account

---

## Version Numbering

Format: MAJOR.MINOR.PATCH

- **MAJOR**: Incompatible API changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, backward compatible

Current: **1.0.0**

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development workflow.

---

## Support

- GitHub Issues: https://github.com/yourusername/ninjatrader-zorro-plugin/issues
- Documentation: `/docs` folder
- Zorro Forum: https://financial-hacker.com

---

## License

MIT License - See [LICENSE](LICENSE) file

---

*This changelog follows the Keep a Changelog format. For detailed commit history, see the Git log.*
