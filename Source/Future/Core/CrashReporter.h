// Source/Core/CrashReporter.h
// Opt-in crash reporting via Sentry Native SDK.
// Gated behind XOCEANUS_CRASH_REPORTING build flag (default OFF).
// User must explicitly enable via preferences — no data sent without consent.

#pragma once

#if XOCEANUS_CRASH_REPORTING
#include <sentry.h>
#endif

namespace xoceanus
{

class CrashReporter
{
public:
    static void initialize()
    {
#if XOCEANUS_CRASH_REPORTING
#ifdef SENTRY_DSN
        sentry_options_t* options = sentry_options_new();
        sentry_options_set_dsn(options, SENTRY_DSN);
        sentry_options_set_release(options, "xoceanus@" XOCEANUS_VERSION_STRING);
        sentry_options_set_environment(options, "production");

        // Privacy: only send crash reports, not breadcrumbs or user data
        sentry_options_set_max_breadcrumbs(options, 0);
        sentry_options_set_require_user_consent(options, 1);

        sentry_init(options);
#endif
#endif
    }

    static void setUserConsent(bool granted)
    {
#if XOCEANUS_CRASH_REPORTING
        if (granted)
            sentry_user_consent_give();
        else
            sentry_user_consent_revoke();
#endif
        (void)granted;
    }

    static void shutdown()
    {
#if XOCEANUS_CRASH_REPORTING
        sentry_close();
#endif
    }
};

} // namespace xoceanus
