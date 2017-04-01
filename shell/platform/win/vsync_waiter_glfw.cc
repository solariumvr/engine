#include "flutter/shell/platform/win/vsync_waiter_glfw.h"
#include "third_party/glfw/include/GLFW/glfw3.h"

namespace shell {
	void VsyncWaiterGlfw::AsyncWaitForVsync(Callback callback)
	{
		callback(ftl::TimePoint::Now());
	}
}