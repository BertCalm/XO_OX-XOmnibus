// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef

import android.app.Application
import android.util.Log

class ObrixReefApplication : Application() {

    override fun onCreate() {
        super.onCreate()
        Log.i("ObrixReef", "Application created — Obrix engine ready")
    }
}
