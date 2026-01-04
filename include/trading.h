// trading.h - Zorro Trading Types
// Minimal subset needed for broker plugin development
// Full version available in Zorro's include folder

#pragma once

#ifndef TRADING_H
#define TRADING_H

#include <windows.h>

// Ensure 4-byte struct alignment for Zorro compatibility
#pragma pack(push, 4)

// DATE type - OLE Automation date (days since Dec 30, 1899)
typedef double DATE;

// Return value for unavailable data
#define NAY (-999999)

// T6 structure for historical price data
typedef struct T6
{
    DATE  time;
    float fHigh;
    float fLow;
    float fOpen;
    float fClose;
    float fVal;
    float fVol;
} T6;

// Broker command codes
#define GET_COMPLIANCE     327
#define GET_MAXTICKS       328
#define GET_MAXREQUESTS    329
#define GET_LOCK           330
#define GET_POSITION       331
#define GET_ACCOUNT        332
#define GET_TIME           333
#define GET_DIGITS         334
#define GET_STOPLEVEL      335
#define GET_TRADEALLOWED   336
#define GET_MINLOT         337
#define GET_LOTSTEP        338
#define GET_MAXLOT         339
#define GET_MARGININIT     340
#define GET_MARGINMAINTAIN 341
#define GET_MARGINHEDGED   342
#define GET_MARGINREQUIRED 343
#define GET_DELAY          344
#define GET_WAIT           345
#define GET_TYPE           346
#define GET_UUID           347
#define GET_BROKERZONE     348
#define GET_PRICETYPE      349
#define GET_VOLTYPE        350
#define GET_NTRADES        356
#define GET_TRADES         357
#define GET_AVGENTRY       358
#define GET_DIAGNOSTICS    359  // Query current diagnostic level

#define SET_PATCH          393
#define SET_DELAY          394
#define SET_WAIT           395
#define SET_LOCK           396
#define SET_SYMBOL         397
#define SET_MAGIC          398
#define SET_MULTIPLIER     399
#define SET_CLASS          400
#define SET_LIMIT          401
#define SET_FUNCTIONS      402
#define SET_ORDERTEXT      403
#define SET_HWND           404
#define SET_COMBO_LEGS     405
#define SET_DIAGNOSTICS    406
#define SET_AMOUNT         407
#define SET_ORDERTYPE      408
#define SET_PRICETYPE      409
#define SET_VOLTYPE        410
#define SET_UUID           411

#define DO_EXERCISE        420
#define DO_CANCEL          421

// Plugin version
#define PLUGIN_VERSION     2

// Order types for SET_ORDERTYPE
#define ORDER_AON          0   // All or None
#define ORDER_GTC          1   // Good Till Cancelled
#define ORDER_IOC          2   // Immediate or Cancel
#define ORDER_FOK          3   // Fill or Kill

// Compliance flags
#define NFA_COMPLIANT      2

#pragma pack(pop)

#endif // TRADING_H
