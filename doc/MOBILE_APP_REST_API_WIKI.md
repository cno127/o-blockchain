# Mobile App REST API (Wiki Edition)

This page consolidates everything about the mobile REST interface so it can be copied into the GitHub wiki as a single reference. It combines the design spec, implementation guide, and completion report that previously lived in multiple `.md` files.

---

## Quick Facts

- **Base URL:** `/rest/api/v1`
- **Response format:** JSON (default)
- **Auth options:** `X-API-Key` header or signed payloads using the user’s public key
- **Coverage:** All **142** O currencies + their fiat counterparts (lists pulled dynamically from `g_currency_registry`)
- **Primary files:**
  - `src/rest/o_mobile_api.h / .cpp`
  - `src/rest.cpp` (URI registration & routing)
  - `doc/*` (this wiki content source)

---

## Endpoint Matrix

| Area | Endpoint | Notes |
| --- | --- | --- |
| **User Lifecycle** | `POST users/register` | Country-restricted registration, returns allowed verification methods |
| | `GET users/{pubkey}/status` | Verification state + reputation |
| | `GET users/{pubkey}/legal-restrictions` | Access/Compliance info per jurisdiction |
| **Exchange Rates** | `GET exchange-rates/{O}` | Current rate, theoretical rate, deviation, stability |
| | `GET exchange-rates/{O}/measured?days=N` | Raw measurements vs theoretical |
| | `GET exchange-rates/{O}/historical?start_date=…&end_date=…` | Daily averages per currency |
| **Map & Stability** | `GET map/countries` | Every fiat currency with avg water price + stability color |
| | `GET map/country/{code}` | Detailed view w/ water + exchange metrics |
| **Wallet (lightweight)** | `GET wallet/{pubkey}/balance` | Placeholder until wallet integration is live |
| | `GET wallet/{pubkey}/transactions?limit=&offset=&currency=` | Placeholder list, ready for wallet hook |
| | `POST wallet/{pubkey}/send` | Stub (requires wallet when available) |
| **Measurements & Notifications** | `GET notifications/{pubkey}/invites` | Active invites filtered by user |
| | `GET notifications/{pubkey}/measurements` | Submission history (needs DB wiring) |
| | `POST measurements/submit/water-price` | Validates invite, prepares transaction payload |
| | `POST measurements/submit/exchange-rate` | Same as above for exchange |
| **General Info** | `GET info/currencies` | Lists all fiat + O currencies from registry |
| | `GET info/stability-status` | Counts stable vs unstable currencies globally |

> **Tip:** All “GET” routes support query strings for paging/filtering; POST routes expect JSON bodies identical to the examples in the original spec.

---

## Implementation Highlights

1. **Central router (`src/rest.cpp`):**
   - Adds custom handlers for `/rest/api/v1/*`
   - Uses helper routers (users, exchange-rates, wallet, notifications, measurements, info) to split shared prefixes
   - Order matters: specific endpoints must precede generic ones in `uri_prefixes`

2. **Endpoint implementation (`src/rest/o_mobile_api.cpp`):**
   - Pulls measurement stats from `g_measurement_system`
   - Enforces geographic policies via `g_geographic_access_control`
   - Validates invites through `g_measurement_db`
   - Auto-discovers all 142 currencies via `g_currency_registry` → `CurrencyExchangeManager::GetSupportedCurrencies`

3. **New helpers inside `MeasurementSystem`:**
   - `GetSupportedOCurrencies()` and `GetSupportedFiatCurrencies()` return sorted, de-duplicated lists derived from the registry
   - All measurement loops (daily averages, auto invitations, stability scans) now iterate over the full currency set

4. **Documentation Files (source for this page):**
   - `doc/MOBILE_APP_REST_API.md` – original endpoint spec
   - `doc/MOBILE_APP_REST_API_IMPLEMENTATION.md` – integration guide
   - `doc/MOBILE_APP_REST_API_COMPLETE.md` – final status + next steps

---

## Deployment / Integration Steps

1. **Register handlers in `src/rest.cpp`:**
   ```cpp
   #include <rest/o_mobile_api.h>
   static const struct {
       const char* prefix;
       bool (*handler)(const std::any&, HTTPRequest*, const std::string&);
   } uri_prefixes[] = {
       // …core endpoints…
       {"/rest/api/v1/users/register", rest_user_register},
       {"/rest/api/v1/users/", rest_api_users_router},
       {"/rest/api/v1/exchange-rates/", rest_api_exchange_rates_router},
       {"/rest/api/v1/map/countries", rest_map_countries},
       {"/rest/api/v1/map/country/", rest_map_country},
       {"/rest/api/v1/wallet/", rest_api_wallet_router},
       {"/rest/api/v1/notifications/", rest_api_notifications_router},
       {"/rest/api/v1/measurements/submit/", rest_api_measurements_router},
       {"/rest/api/v1/info/", rest_api_info_router},
   };
   ```

2. **Update `CMakeLists.txt`:**
   ```cmake
   target_sources(bitcoind PRIVATE rest/o_mobile_api.cpp)
   ```

3. **Build & run with REST enabled:**
   ```bash
   bitcoind -rest -server -printtoconsole
   ```

4. **Manual smoke tests (examples):**
   ```bash
   curl http://localhost:8332/rest/api/v1/users/<pubkey>/status
   curl -X POST http://localhost:8332/rest/api/v1/users/register -H "Content-Type: application/json" -d '{...}'
   curl http://localhost:8332/rest/api/v1/exchange-rates/OUSD
   curl http://localhost:8332/rest/api/v1/map/countries
   curl http://localhost:8332/rest/api/v1/notifications/<pubkey>/invites
   ```

5. **Functional tests (recommended):**
   - Add a new `test/functional/interface_rest_mobile_api.py` covering each route
   - Reuse the REST framework already in `test/functional/interface_rest.py`

---

## Security & Hardening Checklist

- ✅ Input validation on every endpoint
- ✅ Geographic restriction enforcement (KYC tiers, access levels)
- ✅ Invite validity checks prior to measurement submission
- ⚠️ Pending: rate limiting (per IP / per key)
- ⚠️ Pending: API authentication (beyond demo headers)
- ⚠️ Pending: HTTPS enforcement & stricter CORS
- ⚠️ Pending: Wallet integration for balance/transactions/send
- ⚠️ Pending: Real coordinates for map responses
- ⚠️ Pending: Measurement submission should create actual `submitwaterpricetx` / `submitexchangeratetx` transactions instead of placeholders

---

## Performance Notes

- Map/stability endpoints iterate across all 142 currencies; consider caching aggregated data if the mobile app polls frequently.
- Historical queries should be paginated for multi-month ranges (currently returns all results).
- Auto-invite logic uses `GetSupportedFiatCurrencies()` so future currencies are picked up without code changes.

---

## Change Log (high-level)

| Date / Commit | Highlight |
| --- | --- |
| Initial design (`doc/MOBILE_APP_REST_API.md`) | Defined endpoint contracts & payloads |
| Implementation pass (`doc/MOBILE_APP_REST_API_IMPLEMENTATION.md`) | Added core handlers, routing plan |
| Final sweep (`doc/MOBILE_APP_REST_API_COMPLETE.md`) | All endpoints wired, added helper routers + currency registry integration |
| Currency coverage update | Switched every loop to `GetSupportedFiatCurrencies()` / `GetSupportedOCurrencies()` for full 142-currency support |

---

## How to Use This Page

1. Copy the contents into the GitHub wiki (e.g., `Mobile-App-REST-API.md`).
2. Remove the now-redundant `.md` files from `/doc` or replace them with a pointer to the wiki.
3. Keep the wiki version updated as the REST layer evolves.

> **Reminder:** Once the wiki page is live, delete or link the old `/doc/MOBILE_APP_REST_API*.md` files so future contributors don’t edit stale copies.


