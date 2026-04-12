// SPDX-License-Identifier: MIT
// Compile test — verifies ObrixSDKAdapter creates and reports identity.
#include "obrix/ObrixSDKAdapter.h"
#include <cstdio>

int main()
{
    obrix::ObrixSDKAdapter adapter;
    printf("ObrixSDKAdapter created: engine=%s, maxVoices=%d, params=%d\n",
           adapter.getEngineId(),
           adapter.getMaxVoices(),
           adapter.getParameterCount());
    return 0;
}
