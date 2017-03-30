// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.app;

import android.app.Application;

import io.flutter.view.FlutterMain;

/**
 * Flutter implementation of {@link android.app.Application}, managing
 * application-level global initializations.
 */
public class FlutterApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        FlutterMain.startInitialization(this);
    }
}
