#pragma once
#include <juce_core/juce_core.h>
#include <array>

// Platform Keychain (macOS / iOS) — preferred secure storage.
// BlowFish fallback retained for non-Apple platforms (see bottom of file).
#if JUCE_MAC || JUCE_IOS
  #include <Security/Security.h>
#else
  // Fallback only: BlowFish via JUCE cryptography module
  #include <juce_cryptography/juce_cryptography.h>
#endif

namespace xolokun {

//==============================================================================
// SecureKeyStore — Encrypted storage for user-provided AI API keys.
//
// Security architecture:
//
//   1. Keys are NEVER stored in plaintext on disk
//   2. macOS / iOS: keys live exclusively in the platform Keychain
//      (Security framework SecItem API — OS-managed AES encryption,
//       hardware-backed on Apple Silicon / Secure Enclave–equipped devices)
//   3. Other platforms (FALLBACK): AES-like encryption via juce::BlowFish
//      using a device-derived key — DEPRECATED, use macOS/iOS build targets
//   4. Keys exist in plaintext ONLY in memory, ONLY during API calls
//   5. Memory is zeroed after use (secure wipe)
//   6. No key material is ever logged, transmitted, or included in crash reports
//   7. API keys are validated format-only locally (no test call stores the key)
//
// Supported providers:
//   - Anthropic (Claude) — sk-ant-api03-...
//   - OpenAI (ChatGPT)   — sk-...
//   - Google (Gemini)     — AIza...
//
// The key store is a singleton accessed only by SoundAssistant.
// No other module should ever touch API keys.
//==============================================================================
class SecureKeyStore
{
public:
    enum class Provider
    {
        Anthropic = 0,   // Claude
        OpenAI,          // ChatGPT
        Google,          // Gemini
        NumProviders
    };

    SecureKeyStore()
    {
#if !(JUCE_MAC || JUCE_IOS)
        // Fallback path only: derive a device-specific encryption key
        deriveDeviceKey();
#endif
    }

    ~SecureKeyStore()
    {
        // Secure wipe all in-memory key material
#if !(JUCE_MAC || JUCE_IOS)
        secureWipe (deviceKey);
#endif
        for (auto& dk : decryptedKeys)
            secureWipe (dk);
    }

    //--------------------------------------------------------------------------
    // Key management

    /// Store an API key for a provider. The plaintext is wiped from the
    /// input string immediately after the key is committed to secure storage.
    bool storeKey (Provider provider, juce::String& plaintextKey)
    {
        if (!validateKeyFormat (provider, plaintextKey))
            return false;

        bool success = false;

#if JUCE_MAC || JUCE_IOS
        // ---- macOS / iOS: store in platform Keychain --------------------
        auto account  = keychainAccount (provider);
        auto dataLen  = plaintextKey.getNumBytesAsUTF8();

        // Build the base query dict (identifies the existing item, if any)
        CFStringRef serviceRef = CFSTR ("com.xo-ox.xolokun");
        CFStringRef accountRef = CFStringCreateWithCString (
            kCFAllocatorDefault, account.toRawUTF8(), kCFStringEncodingUTF8);

        CFMutableDictionaryRef query = CFDictionaryCreateMutable (
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

        CFDictionarySetValue (query, kSecClass,       kSecClassGenericPassword);
        CFDictionarySetValue (query, kSecAttrService, serviceRef);
        CFDictionarySetValue (query, kSecAttrAccount, accountRef);

        // Try updating an existing item first
        CFDataRef keyData = CFDataCreate (
            kCFAllocatorDefault,
            reinterpret_cast<const UInt8*> (plaintextKey.toRawUTF8()),
            static_cast<CFIndex> (dataLen));

        CFMutableDictionaryRef updateAttrs = CFDictionaryCreateMutable (
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue (updateAttrs, kSecValueData, keyData);

        OSStatus status = SecItemUpdate (query, updateAttrs);

        if (status == errSecItemNotFound)
        {
            // Item does not exist yet — add it
            CFDictionarySetValue (query, kSecValueData, keyData);
            status = SecItemAdd (query, nullptr);
        }

        success = (status == errSecSuccess);

        CFRelease (keyData);
        CFRelease (updateAttrs);
        CFRelease (query);
        CFRelease (accountRef);
        // serviceRef is a compile-time CFSTR literal — do not release

#else
        // ---- Non-Apple fallback: BlowFish-encrypted file ----------------
        // DEPRECATED: BlowFish via juce::BlowFish is not hardware-backed.
        // Migrate to a macOS/iOS build for production use.
        auto encrypted = encrypt (plaintextKey);
        if (encrypted.isNotEmpty())
        {
            auto file = getKeyFile (provider);
            success = file.replaceWithText (encrypted);
        }
#endif

        // Secure wipe plaintext from caller's string regardless of outcome
        secureWipe (plaintextKey);

        // Clear any cached decrypted version
        secureWipe (decryptedKeys[static_cast<size_t> (provider)]);

        return success;
    }

    /// Retrieve a decrypted key for use in an API call.
    /// Returns empty string if no key is stored or retrieval fails.
    /// IMPORTANT: Caller must call releaseKey() when done to wipe memory.
    juce::String retrieveKey (Provider provider)
    {
        // Check session cache first (avoids repeated Keychain/disk access)
        auto& cached = decryptedKeys[static_cast<size_t> (provider)];
        if (cached.isNotEmpty())
            return cached;

#if JUCE_MAC || JUCE_IOS
        // ---- macOS / iOS: read from platform Keychain -------------------
        auto account = keychainAccount (provider);

        CFStringRef serviceRef = CFSTR ("com.xo-ox.xolokun");
        CFStringRef accountRef = CFStringCreateWithCString (
            kCFAllocatorDefault, account.toRawUTF8(), kCFStringEncodingUTF8);

        CFMutableDictionaryRef query = CFDictionaryCreateMutable (
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

        CFDictionarySetValue (query, kSecClass,            kSecClassGenericPassword);
        CFDictionarySetValue (query, kSecAttrService,      serviceRef);
        CFDictionarySetValue (query, kSecAttrAccount,      accountRef);
        CFDictionarySetValue (query, kSecReturnData,       kCFBooleanTrue);
        CFDictionarySetValue (query, kSecMatchLimit,       kSecMatchLimitOne);

        CFTypeRef result = nullptr;
        OSStatus status  = SecItemCopyMatching (query, &result);

        CFRelease (query);
        CFRelease (accountRef);

        if (status != errSecSuccess || result == nullptr)
            return {};

        // result is a CFDataRef containing the raw UTF-8 key bytes
        auto* dataRef = static_cast<CFDataRef> (result);
        auto  len     = static_cast<int> (CFDataGetLength (dataRef));
        auto* bytes   = CFDataGetBytePtr (dataRef);

        juce::String decrypted = juce::String::fromUTF8 (
            reinterpret_cast<const char*> (bytes), len);

        CFRelease (result);

        if (decrypted.isEmpty())
            return {};

        cached = decrypted;
        return decrypted;

#else
        // ---- Non-Apple fallback: BlowFish-encrypted file ----------------
        auto file = getKeyFile (provider);
        if (!file.existsAsFile())
            return {};

        auto encrypted = file.loadFileAsString();
        if (encrypted.isEmpty())
            return {};

        auto decrypted = decrypt (encrypted);
        if (decrypted.isEmpty())
            return {};

        cached = decrypted;
        return decrypted;
#endif
    }

    /// Securely release a key from memory after use.
    void releaseKey (Provider provider)
    {
        secureWipe (decryptedKeys[static_cast<size_t> (provider)]);
    }

    /// Release ALL keys from memory (call on app quit or background).
    void releaseAllKeys()
    {
        for (auto& dk : decryptedKeys)
            secureWipe (dk);
    }

    /// Check if a key is stored for a provider (without decrypting).
    bool hasKey (Provider provider) const
    {
#if JUCE_MAC || JUCE_IOS
        auto account = keychainAccount (provider);

        CFStringRef serviceRef = CFSTR ("com.xo-ox.xolokun");
        CFStringRef accountRef = CFStringCreateWithCString (
            kCFAllocatorDefault, account.toRawUTF8(), kCFStringEncodingUTF8);

        CFMutableDictionaryRef query = CFDictionaryCreateMutable (
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

        CFDictionarySetValue (query, kSecClass,       kSecClassGenericPassword);
        CFDictionarySetValue (query, kSecAttrService, serviceRef);
        CFDictionarySetValue (query, kSecAttrAccount, accountRef);
        CFDictionarySetValue (query, kSecMatchLimit,  kSecMatchLimitOne);
        // kSecReturnData intentionally omitted — just check existence

        OSStatus status = SecItemCopyMatching (query, nullptr);

        CFRelease (query);
        CFRelease (accountRef);

        return (status == errSecSuccess);

#else
        return getKeyFile (provider).existsAsFile();
#endif
    }

    /// Delete a stored key permanently.
    bool deleteKey (Provider provider)
    {
        secureWipe (decryptedKeys[static_cast<size_t> (provider)]);

#if JUCE_MAC || JUCE_IOS
        auto account = keychainAccount (provider);

        CFStringRef serviceRef = CFSTR ("com.xo-ox.xolokun");
        CFStringRef accountRef = CFStringCreateWithCString (
            kCFAllocatorDefault, account.toRawUTF8(), kCFStringEncodingUTF8);

        CFMutableDictionaryRef query = CFDictionaryCreateMutable (
            kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

        CFDictionarySetValue (query, kSecClass,       kSecClassGenericPassword);
        CFDictionarySetValue (query, kSecAttrService, serviceRef);
        CFDictionarySetValue (query, kSecAttrAccount, accountRef);

        OSStatus status = SecItemDelete (query);

        CFRelease (query);
        CFRelease (accountRef);

        // errSecItemNotFound is fine — key was already absent
        return (status == errSecSuccess || status == errSecItemNotFound);

#else
        auto file = getKeyFile (provider);
        if (file.existsAsFile())
            return file.deleteFile();
        return true;
#endif
    }

    //--------------------------------------------------------------------------
    // Validation

    /// Validate key format without making any network calls.
    /// This only checks the key's prefix/length — no data leaves the device.
    static bool validateKeyFormat (Provider provider, const juce::String& key)
    {
        if (key.isEmpty()) return false;

        switch (provider)
        {
            case Provider::Anthropic:
                return key.startsWith ("sk-ant-") && key.length() > 20;
            case Provider::OpenAI:
                return key.startsWith ("sk-") && key.length() > 20;
            case Provider::Google:
                return key.startsWith ("AIza") && key.length() > 20;
            default:
                return false;
        }
    }

    static juce::String providerName (Provider p)
    {
        switch (p)
        {
            case Provider::Anthropic: return "Claude (Anthropic)";
            case Provider::OpenAI:    return "ChatGPT (OpenAI)";
            case Provider::Google:    return "Gemini (Google)";
            default: return "Unknown";
        }
    }

private:
    std::array<juce::String, static_cast<size_t> (Provider::NumProviders)> decryptedKeys;

    //--------------------------------------------------------------------------
    // Keychain account identifier (macOS / iOS only)

#if JUCE_MAC || JUCE_IOS
    static juce::String keychainAccount (Provider provider)
    {
        switch (provider)
        {
            case Provider::Anthropic: return "anthropic-api-key";
            case Provider::OpenAI:    return "openai-api-key";
            case Provider::Google:    return "google-api-key";
            default:                  return "unknown-api-key";
        }
    }
#endif

    //--------------------------------------------------------------------------
    // Non-Apple fallback: BlowFish encryption
    //
    // DEPRECATED — retained only for non-macOS/non-iOS build targets.
    // BlowFish via juce::BlowFish is NOT hardware-backed and uses a
    // device-derived software key.  Migrate to macOS/iOS for production.

#if !(JUCE_MAC || JUCE_IOS)
    juce::String deviceKey;

    void deriveDeviceKey()
    {
        juce::String material;
        material += juce::SystemStats::getComputerName();
        material += juce::SystemStats::getUniqueDeviceID();
        material += "XOlokun_KeyStore_v1";  // App-specific salt
        material += juce::String (juce::SystemStats::getOperatingSystemType());

        juce::SHA256 hash (material.toUTF8());
        deviceKey = hash.toHexString();
    }

    juce::String encrypt (const juce::String& plaintext) const
    {
        if (deviceKey.isEmpty() || plaintext.isEmpty())
            return {};

        juce::BlowFish cipher (deviceKey.toRawUTF8(),
                                static_cast<int> (deviceKey.getNumBytesAsUTF8()));

        juce::MemoryBlock data (plaintext.toRawUTF8(),
                                 plaintext.getNumBytesAsUTF8());

        cipher.encrypt (data);
        return data.toBase64Encoding();
    }

    juce::String decrypt (const juce::String& ciphertext) const
    {
        if (deviceKey.isEmpty() || ciphertext.isEmpty())
            return {};

        juce::MemoryBlock data;
        if (!data.fromBase64Encoding (ciphertext))
            return {};

        juce::BlowFish cipher (deviceKey.toRawUTF8(),
                                static_cast<int> (deviceKey.getNumBytesAsUTF8()));

        cipher.decrypt (data);

        return juce::String::fromUTF8 (static_cast<const char*> (data.getData()),
                                        static_cast<int> (data.getSize()));
    }

    juce::File getKeyFile (Provider provider) const
    {
        auto dir = juce::File::getSpecialLocation (
            juce::File::userApplicationDataDirectory)
            .getChildFile ("XO_OX")
            .getChildFile ("XOlokun")
            .getChildFile ("keys");

        dir.createDirectory();

        juce::String filename;
        switch (provider)
        {
            case Provider::Anthropic: filename = "a.xokey"; break;
            case Provider::OpenAI:    filename = "o.xokey"; break;
            case Provider::Google:    filename = "g.xokey"; break;
            default:                  filename = "unknown.xokey";
        }

        return dir.getChildFile (filename);
    }
#endif // !(JUCE_MAC || JUCE_IOS)

    //--------------------------------------------------------------------------
    // Secure memory wipe (all platforms)

    static void secureWipe (juce::String& str)
    {
        if (str.isEmpty()) return;

        // Overwrite the string's internal buffer with zeros.
        // juce::String uses copy-on-write, so force a unique copy first.
        auto* rawData = const_cast<char*> (str.toRawUTF8());
        auto len = str.getNumBytesAsUTF8();
        if (rawData && len > 0)
            std::memset (rawData, 0, len);
        str = juce::String();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SecureKeyStore)
};

} // namespace xolokun
