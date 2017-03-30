// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/ios/framework/Source/FlutterPlatformPlugin.h"

#include <AudioToolbox/AudioToolbox.h>
#include <Foundation/Foundation.h>
#include <UIKit/UIApplication.h>
#include <UIKit/UIKit.h>

namespace {

constexpr char kTextPlainFormat[] = "text/plain";

NSString* GetDirectoryOfType(NSSearchPathDirectory dir) {
  NSArray* paths =
      NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
  if (paths.count == 0)
    return nil;
  return paths.firstObject;
}

}  // namespaces

namespace shell {

// TODO(abarth): Move these definitions from system_chrome_impl.cc to here.
const char* const kOrientationUpdateNotificationName =
    "io.flutter.plugin.platform.SystemChromeOrientationNotificationName";
const char* const kOrientationUpdateNotificationKey =
    "io.flutter.plugin.platform.SystemChromeOrientationNotificationKey";
const char* const kOverlayStyleUpdateNotificationName =
    "io.flutter.plugin.platform.SystemChromeOverlayNotificationName";
const char* const kOverlayStyleUpdateNotificationKey =
    "io.flutter.plugin.platform.SystemChromeOverlayNotificationKey";

}  // namespace shell

using namespace shell;

@implementation FlutterPlatformPlugin

- (void)handleMethodCall:(FlutterMethodCall*)call resultReceiver:(FlutterResultReceiver)resultReceiver {
  NSString* method = call.method;
  id args = call.arguments;
  if ([method isEqualToString:@"SystemSound.play"]) {
    [self playSystemSound:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"HapticFeedback.vibrate"]) {
    [self vibrateHapticFeedback];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"UrlLauncher.launch"]) {
    [self launchURL:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"SystemChrome.setPreferredOrientations"]) {
    [self setSystemChromePreferredOrientations:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"SystemChrome.setApplicationSwitcherDescription"]) {
    [self setSystemChromeApplicationSwitcherDescription:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"SystemChrome.setEnabledSystemUIOverlays"]) {
    [self setSystemChromeEnabledSystemUIOverlays:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"SystemChrome.setSystemUIOverlayStyle"]) {
    [self setSystemChromeSystemUIOverlayStyle:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"SystemNavigator.pop"]) {
    [self popSystemNavigator];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"Clipboard.getData"]) {
    resultReceiver([self getClipboardData:args]);
  } else if ([method isEqualToString:@"Clipboard.setData"]) {
    [self setClipboardData:args];
    resultReceiver(nil);
  } else if ([method isEqualToString:@"PathProvider.getTemporaryDirectory"]) {
    resultReceiver([self getPathProviderTemporaryDirectory]);
  } else if ([method isEqualToString:@"PathProvider.getApplicationDocumentsDirectory"]) {
    resultReceiver([self getPathProviderApplicationDocumentsDirectory]);
  } else {
    resultReceiver(FlutterMethodNotImplemented);
  }
}

- (void)playSystemSound:(NSString*)soundType {
  if ([soundType isEqualToString:@"SystemSoundType.click"]) {
    // All feedback types are specific to Android and are treated as equal on
    // iOS. The surface must (and does) adopt the UIInputViewAudioFeedback
    // protocol
    [[UIDevice currentDevice] playInputClick];
  }
}

- (void)vibrateHapticFeedback {
  AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
}

- (NSDictionary*)launchURL:(NSString*)urlString {
  NSURL* url = [NSURL URLWithString:urlString];
  UIApplication* application = [UIApplication sharedApplication];
  bool success = [application canOpenURL:url] && [application openURL:url];
  return @{ @"succes": @(success) };
}

- (void)setSystemChromePreferredOrientations:(NSArray*)orientations {
  UIInterfaceOrientationMask mask = 0;

  if (orientations.count == 0) {
    mask |= UIInterfaceOrientationMaskAll;
  } else {
    for (NSString* orientation in orientations) {
      if ([orientation isEqualToString:@"DeviceOrientation.portraitUp"])
        mask |= UIInterfaceOrientationMaskPortrait;
      else if ([orientation isEqualToString:@"DeviceOrientation.portraitDown"])
        mask |= UIInterfaceOrientationMaskPortraitUpsideDown;
      else if ([orientation isEqualToString:@"DeviceOrientation.landscapeLeft"])
        mask |= UIInterfaceOrientationMaskLandscapeLeft;
      else if ([orientation isEqualToString:@"DeviceOrientation.landscapeRight"])
        mask |= UIInterfaceOrientationMaskLandscapeRight;
    }
  }

  if (!mask)
    return;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:@(kOrientationUpdateNotificationName)
                    object:nil
                  userInfo:@{
                    @(kOrientationUpdateNotificationKey) : @(mask)
                  }];

}

- (void)setSystemChromeApplicationSwitcherDescription:(NSDictionary*)object {
  // No counterpart on iOS but is a benign operation. So no asserts.
}

- (void)setSystemChromeEnabledSystemUIOverlays:(NSArray*)overlays {
  // Checks if the top status bar should be visible. This platform ignores all
  // other overlays

  // We opt out of view controller based status bar visibility since we want
  // to be able to modify this on the fly. The key used is
  // UIViewControllerBasedStatusBarAppearance
  [UIApplication sharedApplication].statusBarHidden =
      ![overlays containsObject:@"SystemUiOverlay.top"];
}

- (void)setSystemChromeSystemUIOverlayStyle:(NSString*)style {
  UIStatusBarStyle statusBarStyle;
  if ([style isEqualToString:@"SystemUiOverlayStyle.light"])
    statusBarStyle = UIStatusBarStyleLightContent;
  else if ([style isEqualToString:@"SystemUiOverlayStyle.dark"])
    statusBarStyle = UIStatusBarStyleDefault;
  else
    return;

  NSNumber* infoValue = [[NSBundle mainBundle]
      objectForInfoDictionaryKey:@"UIViewControllerBasedStatusBarAppearance"];
  Boolean delegateToViewController =
      (infoValue == nil || [infoValue boolValue]);

  if (delegateToViewController) {
    // This notification is respected by the iOS embedder
    [[NSNotificationCenter defaultCenter]
        postNotificationName:@(kOverlayStyleUpdateNotificationName)
                      object:nil
                    userInfo:@{
                      @(kOverlayStyleUpdateNotificationKey) : @(statusBarStyle)
                    }];
  } else {
    // Note: -[UIApplication setStatusBarStyle] is deprecated in iOS9
    // in favor of delegating to the view controller
    [[UIApplication sharedApplication] setStatusBarStyle:statusBarStyle];
  }
}

- (void)popSystemNavigator {
  // Apple's human user guidelines say not to terminate iOS applications.
}

- (NSDictionary*)getClipboardData:(NSString*)format {
  UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
  if (!format || [format isEqualToString:@(kTextPlainFormat)])
    return @{ @"text": pasteboard.string };
  return nil;
}

- (void)setClipboardData:(NSDictionary *)data {
  UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
  pasteboard.string = data[@"text"];
}

- (NSString*)getPathProviderTemporaryDirectory {
  return GetDirectoryOfType(NSCachesDirectory);
}

- (NSString*)getPathProviderApplicationDocumentsDirectory {
  return GetDirectoryOfType(NSDocumentDirectory);
}

@end
