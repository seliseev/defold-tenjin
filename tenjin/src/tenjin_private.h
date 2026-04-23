#ifndef DEFOLD_TENJIN_PRIVATE_H
#define DEFOLD_TENJIN_PRIVATE_H

#include <stdbool.h>

namespace dmTenjin
{
    // Real implementations live in tenjin_android.cpp on Android and are
    // provided as empty stubs inside tenjin.cpp on every other platform.
    void Initialize(const char* sdk_key);
    void SetAppStore(const char* store);
    void Connect();

    void SendEvent(const char* name);
    void SendEventWithValue(const char* name, int value);

    void Transaction(const char* product_id, const char* currency_code, int quantity,
                     double unit_price, const char* purchase_data, const char* data_signature);
    void TransactionAmazon(const char* product_id, const char* currency_code, int quantity,
                           double unit_price, const char* receipt_id, const char* user_id,
                           const char* receipt);

    void OptIn();
    void OptOut();
    void OptInParams(const char** params, int params_count);
    void OptOutParams(const char** params, int params_count);
    bool OptInOutUsingCMP();

    void SetGoogleDMAParameters(bool ad_personalization, bool ad_user_data);
    void OptInGoogleDMA();
    void OptOutGoogleDMA();

    void SetCustomerUserId(const char* user_id);
    // Returned pointer is owned by the caller and must be freed with free().
    char* GetCustomerUserId();
    char* GetAnalyticsInstallationId();

    void AppendAppSubversion(int subversion);
    void SetCacheEventSetting(bool enabled);

    // Returned pointer is owned by the caller and must be freed with free().
    char* GetUserProfileJson();
    void ResetUserProfile();

    void EventAdImpression(const char* network, const char* json);
}

#endif
