package com.defold.tenjin;

import android.app.Activity;
import android.util.Log;

import com.tenjin.android.TenjinSDK;
import com.tenjin.android.TenjinSDK.AppStoreType;

import org.json.JSONArray;
import org.json.JSONObject;

import java.lang.reflect.Method;
import java.util.Collection;
import java.util.Map;

/**
 * Thin static bridge between the Defold native-extension JNI layer and the
 * Tenjin Android SDK. All methods are called from the engine main thread via
 * JNI and translate to TenjinSDK API calls. Methods are static and
 * exception-safe: every call catches Throwable and logs it so that a broken
 * Tenjin SDK cannot crash the game process.
 */
public class TenjinJNI {

    private static final String TAG = "TenjinJNI";

    private static Activity sActivity;
    private static TenjinSDK sInstance;

    // ---------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------

    public static void init(Activity activity, String sdkKey) {
        try {
            sActivity = activity;
            sInstance = TenjinSDK.getInstance(activity, sdkKey);
            Log.d(TAG, "init ok");
        } catch (Throwable t) {
            Log.e(TAG, "init failed", t);
        }
    }

    public static void setAppStore(String store) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("setAppStore"); return; }
        try {
            AppStoreType type;
            if ("googleplay".equalsIgnoreCase(store)) {
                type = AppStoreType.googleplay;
            } else if ("amazon".equalsIgnoreCase(store)) {
                type = AppStoreType.amazon;
            } else if ("other".equalsIgnoreCase(store)) {
                type = AppStoreType.other;
            } else {
                type = AppStoreType.unspecified;
            }
            s.setAppStore(type);
        } catch (Throwable t) {
            Log.e(TAG, "setAppStore failed", t);
        }
    }

    public static void connect() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("connect"); return; }
        try {
            s.connect();
        } catch (Throwable t) {
            Log.e(TAG, "connect failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // Events
    // ---------------------------------------------------------------------

    public static void eventWithName(String name) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("eventWithName"); return; }
        try {
            s.eventWithName(name);
        } catch (Throwable t) {
            Log.e(TAG, "eventWithName failed", t);
        }
    }

    public static void eventWithNameAndValue(String name, int value) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("eventWithNameAndValue"); return; }
        try {
            s.eventWithNameAndValue(name, value);
        } catch (Throwable t) {
            Log.e(TAG, "eventWithNameAndValue failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // IAP
    // ---------------------------------------------------------------------

    public static void transaction(String productId, String currencyCode, int quantity,
                                   double unitPrice, String purchaseData, String dataSignature) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("transaction"); return; }
        try {
            s.transaction(productId, currencyCode, quantity, unitPrice, purchaseData, dataSignature);
        } catch (Throwable t) {
            Log.e(TAG, "transaction failed", t);
        }
    }

    public static void transactionAmazon(String productId, String currencyCode, int quantity,
                                         double unitPrice, String receiptId, String userId,
                                         String receipt) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("transactionAmazon"); return; }
        try {
            s.transactionAmazon(productId, currencyCode, quantity, unitPrice, receiptId, userId, receipt);
        } catch (Throwable t) {
            Log.e(TAG, "transactionAmazon failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // GDPR opt-in / opt-out
    // ---------------------------------------------------------------------

    public static void optIn() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optIn"); return; }
        try { s.optIn(); } catch (Throwable t) { Log.e(TAG, "optIn failed", t); }
    }

    public static void optOut() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optOut"); return; }
        try { s.optOut(); } catch (Throwable t) { Log.e(TAG, "optOut failed", t); }
    }

    public static void optInParams(String[] params) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optInParams"); return; }
        try { s.optInParams(params); } catch (Throwable t) { Log.e(TAG, "optInParams failed", t); }
    }

    public static void optOutParams(String[] params) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optOutParams"); return; }
        try { s.optOutParams(params); } catch (Throwable t) { Log.e(TAG, "optOutParams failed", t); }
    }

    public static boolean optInOutUsingCMP() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optInOutUsingCMP"); return false; }
        try {
            return s.optInOutUsingCMP();
        } catch (Throwable t) {
            Log.e(TAG, "optInOutUsingCMP failed", t);
            return false;
        }
    }

    // ---------------------------------------------------------------------
    // Google DMA
    // ---------------------------------------------------------------------

    public static void setGoogleDMAParameters(boolean adPersonalization, boolean adUserData) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("setGoogleDMAParameters"); return; }
        try {
            s.setGoogleDMAParameters(adPersonalization, adUserData);
        } catch (Throwable t) {
            Log.e(TAG, "setGoogleDMAParameters failed", t);
        }
    }

    public static void optInGoogleDMA() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optInGoogleDMA"); return; }
        try { s.optInGoogleDMA(); } catch (Throwable t) { Log.e(TAG, "optInGoogleDMA failed", t); }
    }

    public static void optOutGoogleDMA() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("optOutGoogleDMA"); return; }
        try { s.optOutGoogleDMA(); } catch (Throwable t) { Log.e(TAG, "optOutGoogleDMA failed", t); }
    }

    // ---------------------------------------------------------------------
    // User / misc
    // ---------------------------------------------------------------------

    public static void setCustomerUserId(String userId) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("setCustomerUserId"); return; }
        try { s.setCustomerUserId(userId); } catch (Throwable t) { Log.e(TAG, "setCustomerUserId failed", t); }
    }

    public static String getCustomerUserId() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("getCustomerUserId"); return null; }
        try {
            return s.getCustomerUserId();
        } catch (Throwable t) {
            Log.e(TAG, "getCustomerUserId failed", t);
            return null;
        }
    }

    public static String getAnalyticsInstallationId() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("getAnalyticsInstallationId"); return null; }
        try {
            return s.getAnalyticsInstallationId();
        } catch (Throwable t) {
            Log.e(TAG, "getAnalyticsInstallationId failed", t);
            return null;
        }
    }

    public static void appendAppSubversion(int subversion) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("appendAppSubversion"); return; }
        try { s.appendAppSubversion(subversion); } catch (Throwable t) { Log.e(TAG, "appendAppSubversion failed", t); }
    }

    public static void setCacheEventSetting(boolean enabled) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("setCacheEventSetting"); return; }
        try {
            s.setCacheEventSetting(enabled);
        } catch (Throwable t) {
            Log.e(TAG, "setCacheEventSetting failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // User profile (LiveOps metrics)
    // ---------------------------------------------------------------------

    public static String getUserProfileJson() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("getUserProfileJson"); return null; }
        try {
            Map<String, Object> dict = s.getUserProfileDictionary();
            if (dict == null) {
                return "{}";
            }
            return toJson(dict).toString();
        } catch (Throwable t) {
            Log.e(TAG, "getUserProfileJson failed", t);
            return null;
        }
    }

    public static void resetUserProfile() {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("resetUserProfile"); return; }
        try {
            s.resetUserProfile();
        } catch (Throwable t) {
            Log.e(TAG, "resetUserProfile failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // Impression Level Ad Revenue (ILRD)
    // ---------------------------------------------------------------------

    /**
     * Dispatches an ILRD event to the Tenjin SDK using the per-network
     * {@code eventAdImpression<Network>(JSONObject)} method. Resolved via
     * reflection so the extension does not need compile-time dependencies
     * on the mediation SDKs.
     *
     * @param network one of: "AdMob", "AppLovin", "IronSource", "HyperBid",
     *                "TopOn", "CAS", "TradPlus"
     * @param json    the impression data as a JSON-encoded string
     */
    public static void eventAdImpression(String network, String json) {
        TenjinSDK s = sInstance;
        if (s == null) { warnNotInitialized("eventAdImpression"); return; }
        if (network == null || network.isEmpty()) {
            Log.e(TAG, "eventAdImpression: missing network name");
            return;
        }
        try {
            JSONObject data = (json == null || json.isEmpty()) ? new JSONObject() : new JSONObject(json);
            String methodName = "eventAdImpression" + network;
            Method m = s.getClass().getMethod(methodName, JSONObject.class);
            m.invoke(s, data);
        } catch (NoSuchMethodException nsme) {
            Log.e(TAG, "eventAdImpression: method for network '" + network + "' not found in TenjinSDK. "
                    + "Either the network name is misspelled or your tenjin-android-sdk version is too old.");
        } catch (Throwable t) {
            Log.e(TAG, "eventAdImpression failed", t);
        }
    }

    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------

    private static void warnNotInitialized(String caller) {
        Log.w(TAG, caller + ": Tenjin SDK is not initialised. Call tenjin.init() first.");
    }

    @SuppressWarnings("unchecked")
    private static Object toJson(Object value) throws Exception {
        if (value == null) {
            return JSONObject.NULL;
        }
        if (value instanceof Map) {
            JSONObject obj = new JSONObject();
            for (Map.Entry<String, Object> entry : ((Map<String, Object>) value).entrySet()) {
                obj.put(entry.getKey(), toJson(entry.getValue()));
            }
            return obj;
        }
        if (value instanceof Collection) {
            JSONArray arr = new JSONArray();
            for (Object item : (Collection<?>) value) {
                arr.put(toJson(item));
            }
            return arr;
        }
        if (value instanceof Object[]) {
            JSONArray arr = new JSONArray();
            for (Object item : (Object[]) value) {
                arr.put(toJson(item));
            }
            return arr;
        }
        return value;
    }
}
