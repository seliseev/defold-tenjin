# defold-tenjin

A [Defold](https://defold.com) native extension that wraps the [Tenjin Android SDK](https://github.com/tenjin/tenjin-android-sdk) and exposes it as a Lua module named `tenjin`.

The extension is **Android-only**. On every other platform (iOS, macOS, Windows, Linux, HTML5) all functions are still registered but log a single warning and do nothing, so game code that targets multiple platforms stays portable without `if sys.get_sys_info().system_name == "Android"` guards.

Under the hood:

- Native C++ glue (`tenjin.cpp`, `tenjin_android.cpp`) registered with the  Defold extension system.
- Thin Java helper (`com.defold.tenjin.TenjinJNI`) that wraps `com.tenjin.android.TenjinSDK` and is called via JNI.
- Gradle pulls the latest Tenjin SDK + required Google Play identifiers from Maven Central at build-server time.

---

## Table of contents

- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
  - [Android permissions](#android-permissions)
  - [App store hint](#app-store-hint)
  - [Tenjin SDK key](#tenjin-sdk-key)
- [Quick start](#quick-start)
- [Lua API reference](#lua-api-reference)
  - [Init and lifecycle](#init-and-lifecycle)
  - [Custom events](#custom-events)
  - [In-app purchases](#in-app-purchases)
  - [GDPR opt-in / opt-out](#gdpr-opt-in--opt-out)
  - [Google DMA parameters](#google-dma-parameters)
  - [User identifiers and misc](#user-identifiers-and-misc)
  - [User profile (LiveOps metrics)](#user-profile-liveops-metrics)
  - [Impression Level Ad Revenue (ILRD)](#impression-level-ad-revenue-ilrd)
  - [Constants](#constants)
- [Platform behaviour](#platform-behaviour)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)
- [Out of scope](#out-of-scope)
- [Credits](#credits)

---

## Requirements

- [Defold](https://defold.com) 1.6.0 or newer (any version that supports [native extensions](https://defold.com/manuals/extensions/)).
- An Android API level 21+ target (minSdk 21 is the Tenjin SDK minimum).
- A **Tenjin SDK Key** — obtained from your app's page in the [Tenjin dashboard](https://www.tenjin.com/). Each app can have up to three keys.

Building Android requires nothing beyond a standard Defold setup; the Defold cloud build server resolves all Gradle dependencies for you.

---

## Installation

Open your own Defold project's `game.project` file, go to the **Project → Dependencies** section and add the URL to this repository's latest release archive, for example:

```
https://github.com/<your-org>/defold-tenjin/archive/refs/tags/v1.0.0.zip
```

or, to always track `main`:

```
https://github.com/<your-org>/defold-tenjin/archive/refs/heads/main.zip
```

Then fetch the library:

1. `Project` → `Fetch Libraries` in the Defold editor.
2. Rebuild your project (`Project` → `Build` or `Build HTML5`).

The extension compiles as part of the Defold cloud build, so the first Android build after adding the dependency will take a bit longer while Gradle downloads the Tenjin SDK and its transitive dependencies.

### What gets pulled in

On Android, the following Maven artifacts are added to your bundled APK:


| Artifact                                              | Purpose                                       |
| ----------------------------------------------------- | --------------------------------------------- |
| `com.tenjin:android-sdk`                              | Tenjin SDK itself (install / event tracking). |
| `com.google.android.gms:play-services-ads-identifier` | Android Advertising ID (AAID).                |
| `com.google.android.gms:play-services-appset`         | App Set ID.                                   |
| `com.android.installreferrer:installreferrer`         | Google Play Install Referrer.                 |


ProGuard / R8 rules for Tenjin, Google Play Services, installreferrer,
Gson / TypeToken and OAID providers ship inside the extension
(`[tenjin/manifests/android/tenjin.pro](tenjin/manifests/android/tenjin.pro)`).

---

## Configuration

### Android permissions

`INTERNET` and `ACCESS_NETWORK_STATE` are granted by Defold's default
Android manifest, so no action is needed there. This extension merges in
the following additional entries automatically:

```xml
<uses-permission android:name="com.google.android.gms.permission.AD_ID" />

<queries>
    <package android:name="com.facebook.katana"   />
    <package android:name="com.instagram.android" />
</queries>
```

- `AD_ID` is required by Google Play for apps that read the AAID on
API 33+.
- The two `<queries>` entries allow Tenjin to collect the Meta (Facebook /
Instagram) install referrer.

### App store hint

Tenjin needs to know which store your app is distributed through. Set it either from Lua (recommended):

```lua
tenjin.set_app_store(tenjin.APP_STORE_GOOGLEPLAY)
```

or by merging your own `meta-data` entry into the Android manifest from an extra extension you own:

```xml
<meta-data android:name="TENJIN_APP_STORE" android:value="googleplay" />
```

Valid values:

- `tenjin.APP_STORE_GOOGLEPLAY`
- `tenjin.APP_STORE_AMAZON`
- `tenjin.APP_STORE_OTHER`
- `tenjin.APP_STORE_UNSPECIFIED` (default)

### Tenjin SDK key

Do **not** commit your SDK key to source control for shipping builds.
Common patterns:

- Read it from a build-time config entry:
  ```lua
  local sdk_key = sys.get_config("tenjin.sdk_key", "")
  tenjin.init(sdk_key)
  ```
  then add `tenjin.sdk_key = ...` under a custom `[tenjin]` section in
  your own `game.project` (not checked in).
- Inject it at build time via `bob --settings` or a custom
`game.project` template on your CI.

---

## Quick start

```lua
function init(self)
    tenjin.init("<YOUR_TENJIN_SDK_KEY>")
    tenjin.set_app_store(tenjin.APP_STORE_GOOGLEPLAY)

    -- First connect(). Subsequent resumes are handled automatically.
    tenjin.connect()

    tenjin.send_event("game_started")
    tenjin.send_event_with_value("coins_earned", 100)
end
```

After `tenjin.init()` has been called, the extension automatically calls
`tenjin.connect()` every time the app is resumed
(`dmExtension::EVENT_ID_ACTIVATEAPP`). This matches Tenjin's requirement
that `connect()` is invoked on every Android `Activity.onResume()`.

Turn this off with `tenjin.set_auto_connect(false)` if you prefer to
manage lifecycle yourself.

---

## Lua API reference

All functions are safe to call on any platform. On non-Android builds a
warning is logged and the call is a no-op; getters return `nil` / `false`
/ `0`.

Every function forwards to the corresponding
[Tenjin Android SDK](https://github.com/tenjin/tenjin-android-sdk) method.

### Init and lifecycle

#### `tenjin.init(sdk_key)`

Create the `TenjinSDK` singleton with your app's SDK key. Must be called
before any other `tenjin.*` function.


| Param     | Type     | Notes                                               |
| --------- | -------- | --------------------------------------------------- |
| `sdk_key` | `string` | The key shown in the Tenjin dashboard for your app. |


```lua
tenjin.init("a1b2c3d4-e5f6-7890-abcd-ef1234567890")
```

#### `tenjin.set_app_store(store)`

Tell Tenjin which app store the build targets. Call *before*
`tenjin.connect()`.


| Param   | Type     | Notes                                      |
| ------- | -------- | ------------------------------------------ |
| `store` | `string` | One of the `tenjin.APP_STORE_`* constants. |


```lua
tenjin.set_app_store(tenjin.APP_STORE_GOOGLEPLAY)
```

#### `tenjin.connect()`

Send the install / open event and sync install-referrer / advertising
identifiers. Call this explicitly after `tenjin.init()` on the first
session; subsequent app resumes are handled by the extension when
auto-connect is on.

```lua
tenjin.connect()
```

#### `tenjin.set_auto_connect(enabled)`

Enable or disable the automatic `connect()` on every
`EVENT_ID_ACTIVATEAPP`. Default: `true`.


| Param     | Type      | Notes                             |
| --------- | --------- | --------------------------------- |
| `enabled` | `boolean` | `false` to turn auto-connect off. |


```lua
tenjin.set_auto_connect(false)
```

### Custom events

#### `tenjin.send_event(name)`

Record a named event. Use letters, digits and underscores. Empty or repeated names are not filtered by the extension — mirror whatever the Tenjin dashboard accepts.

```lua
tenjin.send_event("level_1_completed")
```

#### `tenjin.send_event_with_value(name, value)`

Record a named event together with an integer value. Tenjin sums and averages these values in the dashboard — useful for urrency drains, level counts, etc.


| Param   | Type      | Notes                   |
| ------- | --------- | ----------------------- |
| `name`  | `string`  | Event name.             |
| `value` | `integer` | Integer (32-bit) value. |


```lua
tenjin.send_event_with_value("coins_spent", 250)
```

### In-app purchases

Tenjin validates your IAP receipts on its servers. Before you send events, paste your Base64-encoded RSA public key (Google Play) or the Amazon Shared Key into your app settings in the Tenjin dashboard.

#### `tenjin.transaction(product_id, currency_code, quantity, unit_price, purchase_data, data_signature)`

Send a Google Play purchase for validation.


| Param            | Type      | Notes                                                  |
| ---------------- | --------- | ------------------------------------------------------ |
| `product_id`     | `string`  | SKU / product identifier.                              |
| `currency_code`  | `string`  | ISO-4217 code (e.g. `"USD"`).                          |
| `quantity`       | `integer` | Items bought.                                          |
| `unit_price`     | `number`  | Price per unit in the given currency.                  |
| `purchase_data`  | `string`  | `Purchase.getOriginalJson()` from the Billing Library. |
| `data_signature` | `string`  | `Purchase.getSignature()` from the Billing Library.    |


```lua
tenjin.transaction(
    "com.example.coins_pack_small",
    "USD",
    1,
    1.99,
    purchase_data,
    data_signature)
```

#### `tenjin.transaction_amazon(product_id, currency_code, quantity, unit_price, receipt_id, user_id, receipt)`

Send an Amazon App Store purchase for validation.


| Param           | Type      | Notes                                             |
| --------------- | --------- | ------------------------------------------------- |
| `product_id`    | `string`  | SKU.                                              |
| `currency_code` | `string`  | ISO-4217 code (Amazon receipts don't carry this). |
| `quantity`      | `integer` | Items bought.                                     |
| `unit_price`    | `number`  | Price per unit.                                   |
| `receipt_id`    | `string`  | `Receipt.getReceiptId()`.                         |
| `user_id`       | `string`  | `UserData.getUserId()`.                           |
| `receipt`       | `string`  | The full receipt JSON.                            |


### GDPR opt-in / opt-out

By default Tenjin is opted in. Use these functions to respect a user's consent preferences. `opt_out` stops all requests to Tenjin's backend.

#### `tenjin.opt_in()` / `tenjin.opt_out()`

Fully enable / disable Tenjin tracking.

```lua
if user_consented then
    tenjin.opt_in()
else
    tenjin.opt_out()
end
```

#### `tenjin.opt_in_params(params)`

Send **only** the device-related parameters listed in `params`; drop everything else. `advertising_id` is always required by Tenjin.


| Param    | Type              | Notes                                      |
| -------- | ----------------- | ------------------------------------------ |
| `params` | `table of string` | Subset of Tenjin's device-parameter names. |


```lua
tenjin.opt_in_params({ "ip_address", "advertising_id", "limit_ad_tracking", "referrer" })
```

#### `tenjin.opt_out_params(params)`

Send **every** device-related parameter *except* those listed.

```lua
tenjin.opt_out_params({ "locale", "timezone", "build_id" })
```

#### `tenjin.opt_in_out_using_cmp()`

Decide opt-in / opt-out automatically from an IAB TCF 2.0 CMP consent
string stored on the device. Returns the resulting boolean.

```lua
local opted_in = tenjin.opt_in_out_using_cmp()
```

### Google DMA parameters

If you already have a CMP integrated, Tenjin picks DMA consent up automatically. Use these functions only if you want to override  the CMP or roll your own consent flow.

#### `tenjin.set_google_dma_parameters(ad_personalization, ad_user_data)`


| Param                | Type      | Notes                                |
| -------------------- | --------- | ------------------------------------ |
| `ad_personalization` | `boolean` | User consents to ad personalization. |
| `ad_user_data`       | `boolean` | User consents to ad user data.       |


```lua
tenjin.set_google_dma_parameters(true, true)
```

#### `tenjin.opt_in_google_dma()` / `tenjin.opt_out_google_dma()`

Toggle collection of the Google DMA parameters.

```lua
tenjin.opt_in_google_dma()
```

### User identifiers and misc

#### `tenjin.set_customer_user_id(id)`

Attach an arbitrary user identifier to every event Tenjin sends.

```lua
tenjin.set_customer_user_id("user_42")
```

#### `tenjin.get_customer_user_id()`

Return the current customer user id, or `nil` if none was set.

```lua
local uid = tenjin.get_customer_user_id()
```

#### `tenjin.get_analytics_installation_id()`

Return the random per-install analytics id that Tenjin generates and
persists on the device. Useful for correlating Tenjin data with your own
analytics backend.

```lua
local installation_id = tenjin.get_analytics_installation_id()
```

#### `tenjin.append_app_subversion(n)`

Append a numeric subversion to your app version for A/B reporting in
Tenjin DataVault. If your app version is `1.0.1` and `n = 8888`, Tenjin
reports the version as `1.0.1.8888`.


| Param | Type      |
| ----- | --------- |
| `n`   | `integer` |


```lua
tenjin.append_app_subversion(8888)
tenjin.connect()
```

#### `tenjin.set_cache_event_setting(enabled)`

Enable / disable Tenjin's offline retry queue for events and IAP. When
enabled, events that fail due to a network error are retried after the
next successful request.

```lua
tenjin.set_cache_event_setting(true)
```

### User profile (LiveOps metrics)

The Tenjin SDK automatically tracks session counts, session duration, IAP totals and ILRD ad revenue per user and persists them on the device.

#### `tenjin.get_user_profile()`

Return the full user profile as a Lua table. Always-present keys:


| Key                      | Type      | Description                            |
| ------------------------ | --------- | -------------------------------------- |
| `session_count`          | `integer` | Total sessions.                        |
| `total_session_time`     | `integer` | Total session time (milliseconds).     |
| `average_session_length` | `integer` | Average session (milliseconds).        |
| `last_session_length`    | `integer` | Last completed session (milliseconds). |
| `iap_transaction_count`  | `integer` | Total IAP count.                       |
| `total_ilrd_revenue_usd` | `number`  | Total ad revenue in USD.               |


Conditionally present (only when available):


| Key                       | Type      | Description                    |
| ------------------------- | --------- | ------------------------------ |
| `first_session_date`      | `string`  | ISO-8601 timestamp.            |
| `last_session_date`       | `string`  | ISO-8601 timestamp.            |
| `current_session_length`  | `integer` | Active session duration (ms).  |
| `iap_revenue_by_currency` | `table`   | `currency_code -> number` map. |
| `purchased_product_ids`   | `table`   | Sorted list of product ids.    |
| `ilrd_revenue_by_network` | `table`   | `network_name -> number` map.  |


```lua
local profile = tenjin.get_user_profile()
if profile then
    print("sessions:", profile.session_count)
    print("IAP count:", profile.iap_transaction_count)
    print("ILRD USD:", profile.total_ilrd_revenue_usd)
end
```

#### `tenjin.reset_user_profile()`

Clear all locally stored user profile data.

```lua
tenjin.reset_user_profile()
```

### Impression Level Ad Revenue (ILRD)

If you use a mediation SDK (AppLovin MAX, Unity LevelPlay / IronSource,
Google AdMob, TopOn, CAS, TradPlus, HyperBid) you can forward every
impression's revenue to Tenjin. The extension does **not** pull mediation
SDKs in as dependencies — you must integrate them in your own project
and pass the impression payload as a JSON string.

Every network expects a different JSON schema — check the
[Tenjin mediation docs](https://tenjin.com/docs/category/ad-revenue-ad-mediation-setup/)
for the exact fields your network requires.

#### Generic entry point

```lua
tenjin.event_ad_impression(network, json_string)
```


| Param         | Type     | Notes                                                          |
| ------------- | -------- | -------------------------------------------------------------- |
| `network`     | `string` | One of `tenjin.AD_NETWORK_*` constants.                        |
| `json_string` | `string` | JSON-encoded impression payload. Optional; defaults to `"{}"`. |


#### Per-network convenience wrappers

```lua
tenjin.event_ad_impression_admob(json_string)
tenjin.event_ad_impression_applovin(json_string)
tenjin.event_ad_impression_levelplay(json_string)   -- alias for IronSource
tenjin.event_ad_impression_hyperbid(json_string)
tenjin.event_ad_impression_topon(json_string)
tenjin.event_ad_impression_cas(json_string)
tenjin.event_ad_impression_tradplus(json_string)
```

Example (AdMob):

```lua
local payload = json.encode({
    ad_unit_id                   = ad_unit_id,
    currency_code                = "USD",
    response_id                  = response_id,
    value_micros                 = value_micros,
    mediation_adapter_class_name = adapter_class_name,
    precision_type               = precision_type,
})

tenjin.event_ad_impression_admob(payload)
```

Internally the extension calls
`TenjinSDK.eventAdImpression<Network>(JSONObject)` via Java reflection,
so mediation SDKs stay optional and your APK size is not affected by
networks you don't use.

### Constants

String constants exposed on the module to avoid magic strings:

```lua
-- App stores:
tenjin.APP_STORE_GOOGLEPLAY   -- "googleplay"
tenjin.APP_STORE_AMAZON       -- "amazon"
tenjin.APP_STORE_OTHER        -- "other"
tenjin.APP_STORE_UNSPECIFIED  -- "unspecified"

-- Ad networks (for tenjin.event_ad_impression):
tenjin.AD_NETWORK_ADMOB       -- "AdMob"
tenjin.AD_NETWORK_APPLOVIN    -- "AppLovin"
tenjin.AD_NETWORK_LEVELPLAY   -- "IronSource"
tenjin.AD_NETWORK_IRONSOURCE  -- "IronSource"
tenjin.AD_NETWORK_HYPERBID    -- "HyperBid"
tenjin.AD_NETWORK_TOPON       -- "TopOn"
tenjin.AD_NETWORK_CAS         -- "CAS"
tenjin.AD_NETWORK_TRADPLUS    -- "TradPlus"
```

---

## Platform behaviour


| Platform                          | Behaviour                                                                                                                                                    |
| --------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Android                           | Full implementation, all functions forward to `TenjinSDK`.                                                                                                   |
| iOS, macOS, Windows, Linux, HTML5 | All functions are registered, every call logs a single `tenjin.<name> is only supported on Android; ignored` warning and does nothing. Getters return `nil`. |


This lets you share the same game scripts across Android and desktop /
editor builds without extra guards.

---

## Testing

1. Open [Tenjin dashboard → Support → Test Devices](https://www.tenjin.com/dashboard) and add your test device's `advertising_id` (GAID).
2. Open **SDK Live** in the dashboard.
3. Run your game on the device and trigger an event.
4. You should see events appear in the SDK Live log within a few
  seconds.

For local debugging, filter `adb logcat` by the `TenjinJNI` tag:

```
adb logcat -s TenjinJNI:V
```

Any errors in the Java helper (network failures, misconfigured SDK key, reflection errors for an unavailable ILRD network) are logged there.

---

## Troubleshooting

`**tenjin.<name> is only supported on Android; ignored**`

You are running on a non-Android platform. Expected in the Defold editor and on iOS / desktop builds.

`**TenjinJNI.init failed**`

Typical causes:

- SDK key is empty or wrong.
- Google Play Services not available on the test device / emulator. Tenjin falls back to limited tracking, but full attribution requires Play Services.

`**eventAdImpression: method for network '<X>' not found**`

Either the network name is misspelled (the names are case-sensitive and must match Tenjin's Java method suffix: `AdMob`, `AppLovin`, `IronSource`, `HyperBid`, `TopOn`, `CAS`, `TradPlus`) **or** your bundled `com.tenjin:android-sdk` version is older than the one that introduced the method. Update the Gradle version in `[tenjin/manifests/android/build.gradle](tenjin/manifests/android/build.gradle)` if you fork this extension.

**Events not appearing in the dashboard**

- Make sure `tenjin.connect()` runs on every resume. It does automatically when `tenjin.set_auto_connect(true)` (default).
- Confirm that your test device's GAID is listed in *Test Devices* in the dashboard.
- Check `adb logcat -s TenjinJNI:V` for Java-side errors.

---

## Out of scope

- iOS and other platforms (no-op stubs only for now — PRs welcome).
- OAID libraries (MSA / Huawei). ProGuard rules are preserved so you can add them via a separate extension if you ship on non-Google Android stores.
- Deferred deep-link callback handling.

---

## Credits

- [Defold extension manual](https://defold.com/manuals/extensions/) and [Defold Lua API reference](https://defold.com/ref/overview_defoldlua/).
- [Tenjin Android SDK](https://github.com/tenjin/tenjin-android-sdk).
- [Defold Firebase Analytics extension](https://github.com/defold/extension-firebase-analytics) for the Java / JNI bridge pattern this extension follows.

