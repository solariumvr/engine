// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.plugin.common;

/**
 * Command object representing a method call on a {@link FlutterMethodChannel}.
 */
public final class MethodCall {
    /**
     * The name of the called method.
     */
    public final String method;

    /**
     * Arguments for the call.
     */
    public final Object arguments;

    /**
     * Creates a {@link MethodCall} with the specified method name and arguments.
     *
     * @param method the method name String, not null.
     * @param arguments the arguments, a value supported by the channel's message codec.
     */
    public MethodCall(String method, Object arguments) {
        assert method != null;
        this.method = method;
        this.arguments = arguments;
    }
}
