#pragma once
#include "ToolsCommon.h"

namespace lightning::tools {
	enum PrimitiveMeshType : u32 {
		Plane,
		Cube,
		UVSphere,
		ICOSphere,
		Cylinder,
		Capsule,
		count
	};

	struct PrimitiveInitInfo {
		PrimitiveMeshType type;
		u32 segments[3]{ 1, 1, 1 };
		math::v3 size{ 1, 1, 1 };
		u32 lod{ 0 };
	};
}