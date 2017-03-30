// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_CONTENT_HANDLER_APPLICATION_IMPL_H_
#define FLUTTER_CONTENT_HANDLER_APPLICATION_IMPL_H_

#include <memory>

#include "application/services/application_controller.fidl.h"
#include "application/services/application_runner.fidl.h"
#include "application/services/service_provider.fidl.h"
#include "apps/mozart/services/views/view_provider.fidl.h"
#include "dart/runtime/include/dart_api.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fidl/cpp/bindings/binding_set.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/synchronization/waitable_event.h"

namespace flutter_runner {
class App;
class RuntimeHolder;

class ApplicationControllerImpl : public app::ApplicationController,
                                  public app::ServiceProvider,
                                  public mozart::ViewProvider {
 public:
  ApplicationControllerImpl(
      App* app,
      app::ApplicationPackagePtr application,
      app::ApplicationStartupInfoPtr startup_info,
      fidl::InterfaceRequest<app::ApplicationController> controller);

  ~ApplicationControllerImpl() override;

  // |app::ApplicationController| implementation

  void Kill() override;
  void Detach() override;

  // |app::ServiceProvider| implementation

  void ConnectToService(const fidl::String& service_name,
                        mx::channel channel) override;

  // |mozart::ViewProvider| implementation

  void CreateView(
      fidl::InterfaceRequest<mozart::ViewOwner> view_owner_request,
      fidl::InterfaceRequest<app::ServiceProvider> services) override;

  Dart_Port GetUIIsolateMainPort();
  std::string GetUIIsolateName();

 private:
  void StartRuntimeIfReady();

  App* app_;
  fidl::Binding<app::ApplicationController> binding_;

  fidl::BindingSet<app::ServiceProvider> service_provider_bindings_;
  app::ServiceProviderPtr dart_service_provider_;

  fidl::BindingSet<mozart::ViewProvider> view_provider_bindings_;

  std::string url_;
  std::unique_ptr<RuntimeHolder> runtime_holder_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ApplicationControllerImpl);
};

}  // namespace flutter_runner

#endif  // FLUTTER_CONTENT_HANDLER_APPLICATION_IMPL_H_
