#if defined(DM_PLATFORM_ANDROID)

#include <assert.h>
#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include <dmsdk/sdk.h>
#include <dmsdk/dlib/android.h>

#include "tenjin_private.h"

// JNI bridge to com/defold/tenjin/TenjinJNI.
//
// The helper class stays loaded as long as the application classloader
// does, so we cache a global reference the first time we need it and
// reuse it for every subsequent call. jmethodIDs are also cached.

namespace dmTenjin
{
    static const char* TENJIN_JNI_CLASS = "com.defold.tenjin.TenjinJNI";

    static jclass g_TenjinJNIClass = 0;

    static jclass GetTenjinClass(JNIEnv* env)
    {
        if (g_TenjinJNIClass == 0)
        {
            jclass local = dmAndroid::LoadClass(env, TENJIN_JNI_CLASS);
            if (local == 0)
            {
                dmLogError("tenjin: failed to load %s - is TenjinJNI.java compiled into the apk?", TENJIN_JNI_CLASS);
                return 0;
            }
            g_TenjinJNIClass = (jclass) env->NewGlobalRef(local);
        }
        return g_TenjinJNIClass;
    }

    static jmethodID GetStaticMethod(JNIEnv* env, const char* name, const char* sig)
    {
        jclass cls = GetTenjinClass(env);
        if (cls == 0) return 0;
        jmethodID mid = env->GetStaticMethodID(cls, name, sig);
        if (mid == 0)
        {
            dmLogError("tenjin: method %s%s not found on %s", name, sig, TENJIN_JNI_CLASS);
            env->ExceptionClear();
        }
        return mid;
    }

    static jstring NewJString(JNIEnv* env, const char* s)
    {
        return env->NewStringUTF(s ? s : "");
    }

    // ---------------------------------------------------------------------
    // Lifecycle
    // ---------------------------------------------------------------------

    void Initialize(const char* sdk_key)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "init", "(Landroid/app/Activity;Ljava/lang/String;)V");
        if (mid == 0) return;

        jobject activity = thread.GetActivity()->clazz;
        jstring j_sdk_key = NewJString(env, sdk_key);
        env->CallStaticVoidMethod(cls, mid, activity, j_sdk_key);
        env->DeleteLocalRef(j_sdk_key);
    }

    void SetAppStore(const char* store)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "setAppStore", "(Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j = NewJString(env, store);
        env->CallStaticVoidMethod(cls, mid, j);
        env->DeleteLocalRef(j);
    }

    void Connect()
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "connect", "()V");
        if (mid == 0) return;

        env->CallStaticVoidMethod(cls, mid);
    }

    // ---------------------------------------------------------------------
    // Events
    // ---------------------------------------------------------------------

    void SendEvent(const char* name)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "eventWithName", "(Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j = NewJString(env, name);
        env->CallStaticVoidMethod(cls, mid, j);
        env->DeleteLocalRef(j);
    }

    void SendEventWithValue(const char* name, int value)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "eventWithNameAndValue", "(Ljava/lang/String;I)V");
        if (mid == 0) return;

        jstring j = NewJString(env, name);
        env->CallStaticVoidMethod(cls, mid, j, (jint)value);
        env->DeleteLocalRef(j);
    }

    // ---------------------------------------------------------------------
    // IAP
    // ---------------------------------------------------------------------

    void Transaction(const char* product_id, const char* currency_code, int quantity,
                     double unit_price, const char* purchase_data, const char* data_signature)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "transaction",
            "(Ljava/lang/String;Ljava/lang/String;IDLjava/lang/String;Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j_product    = NewJString(env, product_id);
        jstring j_currency   = NewJString(env, currency_code);
        jstring j_purchase   = NewJString(env, purchase_data);
        jstring j_signature  = NewJString(env, data_signature);
        env->CallStaticVoidMethod(cls, mid, j_product, j_currency, (jint)quantity,
                                  (jdouble)unit_price, j_purchase, j_signature);
        env->DeleteLocalRef(j_product);
        env->DeleteLocalRef(j_currency);
        env->DeleteLocalRef(j_purchase);
        env->DeleteLocalRef(j_signature);
    }

    void TransactionAmazon(const char* product_id, const char* currency_code, int quantity,
                           double unit_price, const char* receipt_id, const char* user_id,
                           const char* receipt)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "transactionAmazon",
            "(Ljava/lang/String;Ljava/lang/String;IDLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j_product   = NewJString(env, product_id);
        jstring j_currency  = NewJString(env, currency_code);
        jstring j_receipt_id= NewJString(env, receipt_id);
        jstring j_user_id   = NewJString(env, user_id);
        jstring j_receipt   = NewJString(env, receipt);
        env->CallStaticVoidMethod(cls, mid, j_product, j_currency, (jint)quantity,
                                  (jdouble)unit_price, j_receipt_id, j_user_id, j_receipt);
        env->DeleteLocalRef(j_product);
        env->DeleteLocalRef(j_currency);
        env->DeleteLocalRef(j_receipt_id);
        env->DeleteLocalRef(j_user_id);
        env->DeleteLocalRef(j_receipt);
    }

    // ---------------------------------------------------------------------
    // GDPR
    // ---------------------------------------------------------------------

    static void CallVoidNoArgs(const char* name)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;
        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;
        jmethodID mid = GetStaticMethod(env, name, "()V");
        if (mid == 0) return;
        env->CallStaticVoidMethod(cls, mid);
    }

    void OptIn()  { CallVoidNoArgs("optIn"); }
    void OptOut() { CallVoidNoArgs("optOut"); }

    static void CallStringArray(const char* name, const char** params, int params_count)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, name, "([Ljava/lang/String;)V");
        if (mid == 0) return;

        jclass string_cls = env->FindClass("java/lang/String");
        jobjectArray array = env->NewObjectArray((jsize)params_count, string_cls, 0);
        for (int i = 0; i < params_count; ++i)
        {
            jstring s = NewJString(env, params[i]);
            env->SetObjectArrayElement(array, i, s);
            env->DeleteLocalRef(s);
        }
        env->CallStaticVoidMethod(cls, mid, array);
        env->DeleteLocalRef(array);
        env->DeleteLocalRef(string_cls);
    }

    void OptInParams(const char** params, int params_count)
    {
        CallStringArray("optInParams", params, params_count);
    }

    void OptOutParams(const char** params, int params_count)
    {
        CallStringArray("optOutParams", params, params_count);
    }

    bool OptInOutUsingCMP()
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return false;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return false;

        jmethodID mid = GetStaticMethod(env, "optInOutUsingCMP", "()Z");
        if (mid == 0) return false;

        return env->CallStaticBooleanMethod(cls, mid) == JNI_TRUE;
    }

    // ---------------------------------------------------------------------
    // Google DMA
    // ---------------------------------------------------------------------

    void SetGoogleDMAParameters(bool ad_personalization, bool ad_user_data)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "setGoogleDMAParameters", "(ZZ)V");
        if (mid == 0) return;

        env->CallStaticVoidMethod(cls, mid,
            ad_personalization ? JNI_TRUE : JNI_FALSE,
            ad_user_data       ? JNI_TRUE : JNI_FALSE);
    }

    void OptInGoogleDMA()  { CallVoidNoArgs("optInGoogleDMA"); }
    void OptOutGoogleDMA() { CallVoidNoArgs("optOutGoogleDMA"); }

    // ---------------------------------------------------------------------
    // User id / misc
    // ---------------------------------------------------------------------

    void SetCustomerUserId(const char* user_id)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "setCustomerUserId", "(Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j = NewJString(env, user_id);
        env->CallStaticVoidMethod(cls, mid, j);
        env->DeleteLocalRef(j);
    }

    static char* CallStringNoArgs(const char* name)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return 0;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return 0;

        jmethodID mid = GetStaticMethod(env, name, "()Ljava/lang/String;");
        if (mid == 0) return 0;

        jstring result = (jstring) env->CallStaticObjectMethod(cls, mid);
        if (!result) return 0;

        const char* chars = env->GetStringUTFChars(result, 0);
        char* dup = strdup(chars ? chars : "");
        env->ReleaseStringUTFChars(result, chars);
        env->DeleteLocalRef(result);
        return dup;
    }

    char* GetCustomerUserId()          { return CallStringNoArgs("getCustomerUserId"); }
    char* GetAnalyticsInstallationId() { return CallStringNoArgs("getAnalyticsInstallationId"); }

    void AppendAppSubversion(int subversion)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "appendAppSubversion", "(I)V");
        if (mid == 0) return;

        env->CallStaticVoidMethod(cls, mid, (jint)subversion);
    }

    void SetCacheEventSetting(bool enabled)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "setCacheEventSetting", "(Z)V");
        if (mid == 0) return;

        env->CallStaticVoidMethod(cls, mid, enabled ? JNI_TRUE : JNI_FALSE);
    }

    // ---------------------------------------------------------------------
    // User profile
    // ---------------------------------------------------------------------

    char* GetUserProfileJson() { return CallStringNoArgs("getUserProfileJson"); }

    void ResetUserProfile()    { CallVoidNoArgs("resetUserProfile"); }

    // ---------------------------------------------------------------------
    // ILRD
    // ---------------------------------------------------------------------

    void EventAdImpression(const char* network, const char* json)
    {
        dmAndroid::ThreadAttacher thread;
        JNIEnv* env = thread.GetEnv();
        if (!env) return;

        jclass cls = GetTenjinClass(env);
        if (cls == 0) return;

        jmethodID mid = GetStaticMethod(env, "eventAdImpression",
            "(Ljava/lang/String;Ljava/lang/String;)V");
        if (mid == 0) return;

        jstring j_network = NewJString(env, network);
        jstring j_json    = NewJString(env, json);
        env->CallStaticVoidMethod(cls, mid, j_network, j_json);
        env->DeleteLocalRef(j_network);
        env->DeleteLocalRef(j_json);
    }
}

#endif // DM_PLATFORM_ANDROID
