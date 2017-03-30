// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.view.Choreographer;

public class VsyncWaiter {
    public static void asyncWaitForVsync(final long cookie) {
        Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
            @Override
            public void doFrame(long frameTimeNanos) {
                nativeOnVsync(frameTimeNanos, cookie);
            }
        });
    }

    private static native void nativeOnVsync(long frameTimeNanos, long cookie);
}
