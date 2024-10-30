#include "ToolsCommon.h"

namespace lightning::tools {
	extern void shutdown_texture_tools();
}

EDITOR_INTERFACE void shutdown_content_tools() {
	using namespace lightning::tools;

	shutdown_texture_tools();
}