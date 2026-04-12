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
	SelectionMask,	// 스텐실 쓰기
	PostProcess,	// 아웃라인 그리기
	FXAA,			// 안티앨리어싱
	Editor,			// 에디터 라인
	Grid,			// 그리드
	GizmoOuter,		// 기즈모 외곽
	GizmoInner,		// 기즈모 내부
	Billboard,		// 아이콘 (기즈모보다 나중에 그려서 최상단 보장)
	OverlayFont,	// 스크린 텍스트
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
		"RenderPass::SelectionMask",
		"RenderPass::PostProcess",
		"RenderPass::FXAA",
		"RenderPass::Editor",
		"RenderPass::Grid",
		"RenderPass::GizmoOuter",
		"RenderPass::GizmoInner",
		"RenderPass::Billboard",
		"RenderPass::OverlayFont",
	};
	static_assert(ARRAYSIZE(Names) == (uint32)ERenderPass::MAX, "Names must match ERenderPass entries");
	return Names[(uint32)Pass];
}
