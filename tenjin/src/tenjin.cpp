// tenjin.cpp
// Lua bindings for the Tenjin mobile measurement SDK.
//
// On Android this module forwards every call to com.defold.tenjin.TenjinJNI
// via the JNI bridge in tenjin_android.cpp. On every other platform the Lua
// functions are still registered but act as no-ops so that the same game
// script works unchanged when running on desktop/web/etc.

#define LIB_NAME "TenjinExt"
#define MODULE_NAME "tenjin"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dmsdk/sdk.h>

#include "tenjin_private.h"

namespace
{
    bool g_Initialized   = false;
    bool g_AutoConnect   = true;

#if !defined(DM_PLATFORM_ANDROID)
    void LogUnsupported(const char* name)
    {
        dmLogWarning("tenjin.%s is only supported on Android; ignored.", name);
    }
#endif
}

#if !defined(DM_PLATFORM_ANDROID)
// Stub implementations used on every non-Android platform. They make the
// extension safe to call from cross-platform game code without guarding every
// call with an os.getenv("OS") check.
namespace dmTenjin
{
    void Initialize(const char*)                                           { LogUnsupported("init"); }
    void SetAppStore(const char*)                                          { LogUnsupported("set_app_store"); }
    void Connect()                                                         { LogUnsupported("connect"); }
    void SendEvent(const char*)                                            { LogUnsupported("send_event"); }
    void SendEventWithValue(const char*, int)                              { LogUnsupported("send_event_with_value"); }
    void Transaction(const char*, const char*, int, double, const char*, const char*)
                                                                           { LogUnsupported("transaction"); }
    void TransactionAmazon(const char*, const char*, int, double, const char*, const char*, const char*)
                                                                           { LogUnsupported("transaction_amazon"); }
    void OptIn()                                                           { LogUnsupported("opt_in"); }
    void OptOut()                                                          { LogUnsupported("opt_out"); }
    void OptInParams(const char**, int)                                    { LogUnsupported("opt_in_params"); }
    void OptOutParams(const char**, int)                                   { LogUnsupported("opt_out_params"); }
    bool OptInOutUsingCMP()                                                { LogUnsupported("opt_in_out_using_cmp"); return false; }
    void SetGoogleDMAParameters(bool, bool)                                { LogUnsupported("set_google_dma_parameters"); }
    void OptInGoogleDMA()                                                  { LogUnsupported("opt_in_google_dma"); }
    void OptOutGoogleDMA()                                                 { LogUnsupported("opt_out_google_dma"); }
    void SetCustomerUserId(const char*)                                    { LogUnsupported("set_customer_user_id"); }
    char* GetCustomerUserId()                                              { LogUnsupported("get_customer_user_id"); return 0; }
    char* GetAnalyticsInstallationId()                                     { LogUnsupported("get_analytics_installation_id"); return 0; }
    void AppendAppSubversion(int)                                          { LogUnsupported("append_app_subversion"); }
    void SetCacheEventSetting(bool)                                        { LogUnsupported("set_cache_event_setting"); }
    char* GetUserProfileJson()                                             { LogUnsupported("get_user_profile"); return 0; }
    void ResetUserProfile()                                                { LogUnsupported("reset_user_profile"); }
    void EventAdImpression(const char*, const char*)                       { LogUnsupported("event_ad_impression"); }
}
#endif

// ---------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------

static const char* CheckOptString(lua_State* L, int idx)
{
    if (lua_isnoneornil(L, idx))
        return "";
    return luaL_checkstring(L, idx);
}

// Push a JSON string as a Lua table (object/array/primitives). On success
// the decoded Lua value is left on the top of the stack and the function
// returns true. On failure a nil is pushed and false is returned.
static bool PushJsonAsLua(lua_State* L, const char* json)
{
    if (!json)
    {
        lua_pushnil(L);
        return false;
    }

    lua_getglobal(L, "json");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        dmLogError("tenjin: global 'json' module is missing");
        lua_pushnil(L);
        return false;
    }

    lua_getfield(L, -1, "decode");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        dmLogError("tenjin: json.decode is not available");
        lua_pushnil(L);
        return false;
    }

    lua_pushstring(L, json);
    if (lua_pcall(L, 1, 1, 0) != 0)
    {
        const char* err = lua_tostring(L, -1);
        dmLogError("tenjin: failed to decode user profile JSON: %s", err ? err : "<unknown error>");
        lua_pop(L, 2); // error + json table
        lua_pushnil(L);
        return false;
    }

    // Stack after successful pcall: [..., json_table, decoded_value]
    // Remove json_table and keep decoded_value as return value.
    lua_remove(L, -2);
    return true;
}

// Lua stack-check friendly helper for returning a C string. If s is null or
// empty, pushes nil instead. Frees s afterwards.
static int PushAndFreeCString(lua_State* L, char* s)
{
    if (s == 0 || s[0] == '\0')
    {
        lua_pushnil(L);
    }
    else
    {
        lua_pushstring(L, s);
    }
    if (s) free(s);
    return 1;
}

// ---------------------------------------------------------------------
// Lua API
// ---------------------------------------------------------------------

static int Tenjin_Init(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* sdk_key = luaL_checkstring(L, 1);
    dmTenjin::Initialize(sdk_key);
    g_Initialized = true;
    return 0;
}

static int Tenjin_SetAppStore(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* store = luaL_checkstring(L, 1);
    dmTenjin::SetAppStore(store);
    return 0;
}

static int Tenjin_Connect(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::Connect();
    return 0;
}

static int Tenjin_SetAutoConnect(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    g_AutoConnect = lua_toboolean(L, 1) != 0;
    return 0;
}

static int Tenjin_SendEvent(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* name = luaL_checkstring(L, 1);
    dmTenjin::SendEvent(name);
    return 0;
}

static int Tenjin_SendEventWithValue(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* name = luaL_checkstring(L, 1);
    int value = (int) luaL_checkinteger(L, 2);
    dmTenjin::SendEventWithValue(name, value);
    return 0;
}

static int Tenjin_Transaction(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* product_id      = luaL_checkstring(L, 1);
    const char* currency_code   = luaL_checkstring(L, 2);
    int         quantity        = (int) luaL_checkinteger(L, 3);
    double      unit_price      = luaL_checknumber(L, 4);
    const char* purchase_data   = CheckOptString(L, 5);
    const char* data_signature  = CheckOptString(L, 6);
    dmTenjin::Transaction(product_id, currency_code, quantity, unit_price, purchase_data, data_signature);
    return 0;
}

static int Tenjin_TransactionAmazon(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* product_id      = luaL_checkstring(L, 1);
    const char* currency_code   = luaL_checkstring(L, 2);
    int         quantity        = (int) luaL_checkinteger(L, 3);
    double      unit_price      = luaL_checknumber(L, 4);
    const char* receipt_id      = CheckOptString(L, 5);
    const char* user_id         = CheckOptString(L, 6);
    const char* receipt         = CheckOptString(L, 7);
    dmTenjin::TransactionAmazon(product_id, currency_code, quantity, unit_price, receipt_id, user_id, receipt);
    return 0;
}

static int Tenjin_OptIn(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::OptIn();
    return 0;
}

static int Tenjin_OptOut(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::OptOut();
    return 0;
}

// Helper: read a sequence of strings from a Lua table at `idx` into a
// heap-allocated array of C strings. The caller must free the returned
// array (not the strings; those point into the Lua string pool).
static const char** ReadStringArray(lua_State* L, int idx, int* out_count)
{
    luaL_checktype(L, idx, LUA_TTABLE);
    int count = (int) lua_objlen(L, idx);
    const char** arr = (const char**) malloc(sizeof(const char*) * (count > 0 ? count : 1));
    for (int i = 0; i < count; ++i)
    {
        lua_rawgeti(L, idx, i + 1);
        if (lua_isstring(L, -1))
        {
            arr[i] = lua_tostring(L, -1);
        }
        else
        {
            arr[i] = "";
        }
        lua_pop(L, 1);
    }
    *out_count = count;
    return arr;
}

static int Tenjin_OptInParams(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int n = 0;
    const char** arr = ReadStringArray(L, 1, &n);
    dmTenjin::OptInParams(arr, n);
    free(arr);
    return 0;
}

static int Tenjin_OptOutParams(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int n = 0;
    const char** arr = ReadStringArray(L, 1, &n);
    dmTenjin::OptOutParams(arr, n);
    free(arr);
    return 0;
}

static int Tenjin_OptInOutUsingCMP(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    lua_pushboolean(L, dmTenjin::OptInOutUsingCMP() ? 1 : 0);
    return 1;
}

static int Tenjin_SetGoogleDMAParameters(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    bool ap = lua_toboolean(L, 1) != 0;
    bool au = lua_toboolean(L, 2) != 0;
    dmTenjin::SetGoogleDMAParameters(ap, au);
    return 0;
}

static int Tenjin_OptInGoogleDMA(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::OptInGoogleDMA();
    return 0;
}

static int Tenjin_OptOutGoogleDMA(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::OptOutGoogleDMA();
    return 0;
}

static int Tenjin_SetCustomerUserId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* user_id = luaL_checkstring(L, 1);
    dmTenjin::SetCustomerUserId(user_id);
    return 0;
}

static int Tenjin_GetCustomerUserId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    char* s = dmTenjin::GetCustomerUserId();
    return PushAndFreeCString(L, s);
}

static int Tenjin_GetAnalyticsInstallationId(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    char* s = dmTenjin::GetAnalyticsInstallationId();
    return PushAndFreeCString(L, s);
}

static int Tenjin_AppendAppSubversion(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    int subversion = (int) luaL_checkinteger(L, 1);
    dmTenjin::AppendAppSubversion(subversion);
    return 0;
}

static int Tenjin_SetCacheEventSetting(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    dmTenjin::SetCacheEventSetting(lua_toboolean(L, 1) != 0);
    return 0;
}

static int Tenjin_GetUserProfile(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    char* json = dmTenjin::GetUserProfileJson();
    if (!json || json[0] == '\0')
    {
        lua_pushnil(L);
        if (json) free(json);
        return 1;
    }
    PushJsonAsLua(L, json);
    free(json);
    return 1;
}

static int Tenjin_ResetUserProfile(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmTenjin::ResetUserProfile();
    return 0;
}

static int Tenjin_EventAdImpression(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* network = luaL_checkstring(L, 1);
    const char* json    = CheckOptString(L, 2);
    dmTenjin::EventAdImpression(network, json);
    return 0;
}

// Per-network convenience wrappers. Each one just forwards to
// EventAdImpression with a hard-coded network name so game code can say
// `tenjin.event_ad_impression_admob(json)` instead of stringly-typed calls.
#define TENJIN_ILRD_WRAPPER(lua_name, network_name)                         \
    static int Tenjin_ ## lua_name(lua_State* L)                             \
    {                                                                        \
        DM_LUA_STACK_CHECK(L, 0);                                           \
        const char* json = CheckOptString(L, 1);                            \
        dmTenjin::EventAdImpression(network_name, json);                    \
        return 0;                                                            \
    }

TENJIN_ILRD_WRAPPER(EventAdImpressionAdMob,     "AdMob")
TENJIN_ILRD_WRAPPER(EventAdImpressionAppLovin,  "AppLovin")
TENJIN_ILRD_WRAPPER(EventAdImpressionLevelPlay, "IronSource")
TENJIN_ILRD_WRAPPER(EventAdImpressionHyperBid,  "HyperBid")
TENJIN_ILRD_WRAPPER(EventAdImpressionTopOn,     "TopOn")
TENJIN_ILRD_WRAPPER(EventAdImpressionCAS,       "CAS")
TENJIN_ILRD_WRAPPER(EventAdImpressionTradPlus,  "TradPlus")

#undef TENJIN_ILRD_WRAPPER

// ---------------------------------------------------------------------
// Lua registration
// ---------------------------------------------------------------------

static const luaL_reg Module_methods[] =
{
    {"init",                         Tenjin_Init},
    {"set_app_store",                Tenjin_SetAppStore},
    {"connect",                      Tenjin_Connect},
    {"set_auto_connect",             Tenjin_SetAutoConnect},

    {"send_event",                   Tenjin_SendEvent},
    {"send_event_with_value",        Tenjin_SendEventWithValue},

    {"transaction",                  Tenjin_Transaction},
    {"transaction_amazon",           Tenjin_TransactionAmazon},

    {"opt_in",                       Tenjin_OptIn},
    {"opt_out",                      Tenjin_OptOut},
    {"opt_in_params",                Tenjin_OptInParams},
    {"opt_out_params",               Tenjin_OptOutParams},
    {"opt_in_out_using_cmp",         Tenjin_OptInOutUsingCMP},

    {"set_google_dma_parameters",    Tenjin_SetGoogleDMAParameters},
    {"opt_in_google_dma",            Tenjin_OptInGoogleDMA},
    {"opt_out_google_dma",           Tenjin_OptOutGoogleDMA},

    {"set_customer_user_id",         Tenjin_SetCustomerUserId},
    {"get_customer_user_id",         Tenjin_GetCustomerUserId},
    {"get_analytics_installation_id",Tenjin_GetAnalyticsInstallationId},

    {"append_app_subversion",        Tenjin_AppendAppSubversion},
    {"set_cache_event_setting",      Tenjin_SetCacheEventSetting},

    {"get_user_profile",             Tenjin_GetUserProfile},
    {"reset_user_profile",           Tenjin_ResetUserProfile},

    {"event_ad_impression",          Tenjin_EventAdImpression},
    {"event_ad_impression_admob",    Tenjin_EventAdImpressionAdMob},
    {"event_ad_impression_applovin", Tenjin_EventAdImpressionAppLovin},
    {"event_ad_impression_levelplay",Tenjin_EventAdImpressionLevelPlay},
    {"event_ad_impression_hyperbid", Tenjin_EventAdImpressionHyperBid},
    {"event_ad_impression_topon",    Tenjin_EventAdImpressionTopOn},
    {"event_ad_impression_cas",      Tenjin_EventAdImpressionCAS},
    {"event_ad_impression_tradplus", Tenjin_EventAdImpressionTradPlus},

    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    luaL_register(L, MODULE_NAME, Module_methods);

    #define SET_STRING_CONST(name, value) \
        lua_pushstring(L, value);         \
        lua_setfield(L, -2, name);

    SET_STRING_CONST("APP_STORE_GOOGLEPLAY",  "googleplay")
    SET_STRING_CONST("APP_STORE_AMAZON",      "amazon")
    SET_STRING_CONST("APP_STORE_OTHER",       "other")
    SET_STRING_CONST("APP_STORE_UNSPECIFIED", "unspecified")

    SET_STRING_CONST("AD_NETWORK_ADMOB",      "AdMob")
    SET_STRING_CONST("AD_NETWORK_APPLOVIN",   "AppLovin")
    SET_STRING_CONST("AD_NETWORK_LEVELPLAY",  "IronSource")
    SET_STRING_CONST("AD_NETWORK_IRONSOURCE", "IronSource")
    SET_STRING_CONST("AD_NETWORK_HYPERBID",   "HyperBid")
    SET_STRING_CONST("AD_NETWORK_TOPON",      "TopOn")
    SET_STRING_CONST("AD_NETWORK_CAS",        "CAS")
    SET_STRING_CONST("AD_NETWORK_TRADPLUS",   "TradPlus")

    #undef SET_STRING_CONST

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

// ---------------------------------------------------------------------
// Extension entry points
// ---------------------------------------------------------------------

static dmExtension::Result AppInitializeTenjin(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeTenjin(dmExtension::Params* params)
{
    LuaInit(params->m_L);
    dmLogInfo("Registered extension %s", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeTenjin(dmExtension::AppParams* params)
{
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeTenjin(dmExtension::Params* params)
{
    return dmExtension::RESULT_OK;
}

static void OnEventTenjin(dmExtension::Params* params, const dmExtension::Event* event)
{
    if (event == 0) return;
    switch (event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            if (g_Initialized && g_AutoConnect)
            {
                dmTenjin::Connect();
            }
            break;
        default:
            break;
    }
}

// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)
// The symbol must match ext.manifest `name`.
DM_DECLARE_EXTENSION(TenjinExt, LIB_NAME,
    AppInitializeTenjin, AppFinalizeTenjin,
    InitializeTenjin, 0, OnEventTenjin, FinalizeTenjin)
