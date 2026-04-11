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
	Translucent,
	SelectionMask,
	PostProcess,    // Fog & Outline
	Font,			// World Text
	SubUV,			// Sprites/Particles
	Billboard,		// Billboard Icons
	Editor,
	Grid,
	GizmoOuter,
	GizmoInner,
	OverlayFont,
	MAX
};

inline const char* GetRenderPassName(ERenderPass Pass)
{
	static const char* Names[] = {
		"RenderPass::Opaque",
		"RenderPass::Translucent",
		"RenderPass::SelectionMask",
		"RenderPass::PostProcess",
		"RenderPass::Font",
		"RenderPass::SubUV",
		"RenderPass::Billboard",
		"RenderPass::Editor",
		"RenderPass::Grid",
		"RenderPass::GizmoOuter",
		"RenderPass::GizmoInner",
		"RenderPass::OverlayFont",
	};
	static_assert(ARRAYSIZE(Names) == (uint32)ERenderPass::MAX, "Names must match ERenderPass entries");
	return Names[(uint32)Pass];
}