#pragma once

//	Windows API Include
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>

//	D3D API Include
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_5.h>

#pragma comment(lib, "dxgi")
#include "Core/CoreTypes.h"
#include "Math/Vector.h"

//	Mesh Shape Enum — MeshBufferManager 조회용 (순수 기하 형상)
enum class EMeshShape
{
	Cube,
	Sphere,
	Plane,
	Quad,
	TransGizmo,
	RotGizmo,
	ScaleGizmo,
};

enum class ERenderPass : uint32
{
	Opaque,
	Decal,
	Font,
	SubUV,
	Translucent,
	Fog,
	Editor,         // 축(Axis) 등
	Grid,           // 그리드
	SelectionMask,	// 아웃라인 마스크 생성
	PostProcess,	// 아웃라인 그리기
	Billboard,		// 아이콘
	FXAA,           // 후처리 (기즈모 제외)
	GizmoOuter,
	GizmoInner,
	OverlayFont,
	MAX
};

inline const char* GetRenderPassName(ERenderPass Pass)
{
	static const char* Names[] = {
		"RenderPass::Opaque",
		"RenderPass::Decal",
		"RenderPass::Font",
		"RenderPass::SubUV",
		"RenderPass::Translucent",
		"RenderPass::Fog",
		"RenderPass::Editor",
		"RenderPass::Grid",
		"RenderPass::SelectionMask",
		"RenderPass::PostProcess",
		"RenderPass::Billboard",
		"RenderPass::FXAA",
		"RenderPass::GizmoOuter",
		"RenderPass::GizmoInner",
		"RenderPass::OverlayFont",
	};
	static_assert(ARRAYSIZE(Names) == (uint32)ERenderPass::MAX, "Names must match ERenderPass entries");
	return Names[(uint32)Pass];
}
