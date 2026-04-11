#include "MeshDecalComponent.h"
#include "StaticMeshComponent.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Engine/Runtime/Engine.h"
#include "Mesh/ObjManager.h"
#include "Render/Proxy/DecalMeshSceneProxy.h"
#include "Engine/Texture/Texture2D.h"
#include "Editor/UI/EditorConsoleWidget.h"
#include <cstring>

IMPLEMENT_CLASS(UMeshDecalComponent, UPrimitiveComponent)
namespace
{
	constexpr float MeshDecalSurfaceOffset = 0.001f;

	FVector2 ComputeDecalProjectedUV(const FVector& DecalLocalPos, const FVector& HalfExtent)
	{
		const float U = 0.5f + (HalfExtent.Y != 0.0f ? (DecalLocalPos.Y / (HalfExtent.Y * 2.0f)) : 0.0f);
		const float V = 0.5f - (HalfExtent.Z != 0.0f ? (DecalLocalPos.Z / (HalfExtent.Z * 2.0f)) : 0.0f);
		return FVector2(U, V);
	}

	FVertexPNCT MakeDecalVertex(const FVector& DecalLocalPos, const FVector& DecalLocalNormal, const FVector& HalfExtent)
	{
		FVertexPNCT Vertex = {};
		Vertex.Normal = DecalLocalNormal.Normalized();
		Vertex.Position = DecalLocalPos + Vertex.Normal * MeshDecalSurfaceOffset;
		Vertex.Color = FVector4(1.f, 1.f, 1.f, 1.f);
		Vertex.UV = ComputeDecalProjectedUV(DecalLocalPos, HalfExtent);
		return Vertex;
	}

	bool IsDecalFacingTriangle(const FVector& N0, const FVector& N1, const FVector& N2)
	{
		constexpr float ProjectionDotThreshold = -0.2f;
		FVector AvgNormal = (N0.Normalized() + N1.Normalized() + N2.Normalized()) / 3.0f;
		AvgNormal.Normalize();
		return AvgNormal.Dot(FVector(1.f, 0.f, 0.f)) <= ProjectionDotThreshold;
	}
}

FPrimitiveSceneProxy* UMeshDecalComponent::CreateSceneProxy()
{
	return new FDecalMeshSceneProxy(this);
}

UMeshDecalComponent::UMeshDecalComponent()
{
	LocalExtents = HalfExtent;
}

void UMeshDecalComponent::RefreshMaterial()
{
	if (MaterialSlot.Path.empty() || MaterialSlot.Path == "None")
	{
		Material = nullptr;
		return;
	}

	Material = FObjManager::GetOrLoadMaterial(MaterialSlot.Path);
	if (!Material)
	{
		return;
	}

	if (!Material->DiffuseTexture && !Material->DiffuseTextureFilePath.empty() && GEngine)
	{
		ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
		if (Device)
		{
			Material->DiffuseTexture = UTexture2D::LoadFromFile(Material->DiffuseTextureFilePath, Device);
		}
	}
}

void UMeshDecalComponent::GetEditableProperties(TArray<FPropertyDescriptor>& OutProps)
{
	UPrimitiveComponent::GetEditableProperties(OutProps);
	OutProps.push_back({ "Material", EPropertyType::MaterialSlot, &MaterialSlot });
	OutProps.push_back({ "HalfExtent", EPropertyType::Vec3, &HalfExtent });
}

void UMeshDecalComponent::PostEditProperty(const char* PropertyName)
{
	UPrimitiveComponent::PostEditProperty(PropertyName);

	if (strcmp(PropertyName, "Material") == 0)
	{
		RefreshMaterial();
		MarkProxyDirty(EDirtyFlag::Mesh);
	}
	else if (strcmp(PropertyName, "HalfExtent") == 0)
	{
		LocalExtents = HalfExtent;
		MarkWorldBoundsDirty();
		UpdateDecalMeshData();
	}
}

void UMeshDecalComponent::PostDuplicate()
{
	UPrimitiveComponent::PostDuplicate();
	LocalExtents = HalfExtent;
	RefreshMaterial();
	UpdateDecalMeshData();
}

void UMeshDecalComponent::OnTransformDirty()
{
	UPrimitiveComponent::OnTransformDirty();
	UpdateDecalMeshData();
}


TArray<UPrimitiveComponent*> UMeshDecalComponent::GetCandidates() const
{
	FOBB OBB = FOBB::FromAABB(GetWorldBoundingBox());
	FMatrix Rot = GetRelativeRotation().ToMatrix();
	OBB.Axes[0] = GetForwardVector().Normalized();
	OBB.Axes[1] = GetRightVector().Normalized();
	OBB.Axes[2] = GetUpVector().Normalized();
	return GetOwner()->GetWorld()->QueryOBB(OBB);
}

void UMeshDecalComponent::UpdateDecalMeshData()
{
	DecalMeshData.Indices.clear();
	DecalMeshData.Vertices.clear();
	MeshBuffer.Release();

	FMatrix WorldToDecalLocal = GetWorldMatrix().GetInverse();

	//SAT를 통해 후보가 될 Primitive들을 가져오기.
	TArray<UPrimitiveComponent*> Candidates = GetCandidates();
	UE_LOG("MeshDecal Candidates: %d", static_cast<int>(Candidates.size()));

	for (UPrimitiveComponent* Primitive : Candidates)
	{
		if (Primitive->GetOwner() == GetOwner()) continue;
		UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(Primitive);
		if (!StaticMeshComp) continue;

		FMatrix PrimitiveWorld = StaticMeshComp->GetWorldMatrix();
		TArray<FNormalVertex>& OriginVertices = StaticMeshComp->GetStaticMesh()->GetStaticMeshAsset()->Vertices;
		TArray<uint32>& OriginIndices = StaticMeshComp->GetStaticMesh()->GetStaticMeshAsset()->Indices;

		for (int i = 0; i + 2 < OriginIndices.size(); i += 3)
		{
			//매쉬의 로컬에서의 정점 좌표와 노말
			FVector PrimOriginPos0 = OriginVertices[OriginIndices[i]].pos;
			FVector PrimOriginPos1 = OriginVertices[OriginIndices[i + 1]].pos;
			FVector PrimOriginPos2 = OriginVertices[OriginIndices[i + 2]].pos;

			FVector PrimOriginNormal0 = OriginVertices[OriginIndices[i]].normal;
			FVector PrimOriginNormal1 = OriginVertices[OriginIndices[i + 1]].normal;
			FVector PrimOriginNormal2 = OriginVertices[OriginIndices[i + 2]].normal;

			//매쉬의 월드에서의 정점 좌표와 노말
			FVector PrimWorldPos0 = PrimitiveWorld.TransformPositionWithW(PrimOriginPos0);
			FVector PrimWorldPos1 = PrimitiveWorld.TransformPositionWithW(PrimOriginPos1);
			FVector PrimWorldPos2 = PrimitiveWorld.TransformPositionWithW(PrimOriginPos2);

			FVector PrimNormal0 = PrimitiveWorld.TransformVector(PrimOriginNormal0);
			FVector PrimNormal1 = PrimitiveWorld.TransformVector(PrimOriginNormal1);
			FVector PrimNormal2 = PrimitiveWorld.TransformVector(PrimOriginNormal2);

			//매쉬의 DecalLocal에서의 정점 좌표와 노말
			FVector PrimDecalPos0 = WorldToDecalLocal.TransformPositionWithW(PrimWorldPos0);
			FVector PrimDecalPos1 = WorldToDecalLocal.TransformPositionWithW(PrimWorldPos1);
			FVector PrimDecalPos2 = WorldToDecalLocal.TransformPositionWithW(PrimWorldPos2);

			FVector PrimDecalNormal0 = WorldToDecalLocal.TransformVector(PrimNormal0);
			FVector PrimDecalNormal1 = WorldToDecalLocal.TransformVector(PrimNormal1);
			FVector PrimDecalNormal2 = WorldToDecalLocal.TransformVector(PrimNormal2);

			if (!IsDecalFacingTriangle(PrimDecalNormal0, PrimDecalNormal1, PrimDecalNormal2))
			{
				continue;
			}

			FVertexPNCT PrimDecalVertex0 = MakeDecalVertex(PrimDecalPos0, PrimDecalNormal0, HalfExtent);
			FVertexPNCT PrimDecalVertex1 = MakeDecalVertex(PrimDecalPos1, PrimDecalNormal1, HalfExtent);
			FVertexPNCT PrimDecalVertex2 = MakeDecalVertex(PrimDecalPos2, PrimDecalNormal2, HalfExtent);

			TArray<FVertexPNCT> ClippedVertices;
			ClipByDecalLocalOBB(ClippedVertices, PrimDecalVertex0, PrimDecalVertex1, PrimDecalVertex2);

			//정점이 3개 이하면 다각형이 아니니 패스
			if (ClippedVertices.size() < 3)
			{
				continue;
			}

			const uint32 BaseVertexIndex = static_cast<uint32>(DecalMeshData.Vertices.size());
			DecalMeshData.Vertices.insert(DecalMeshData.Vertices.end(), ClippedVertices.begin(), ClippedVertices.end());

			for (uint32 FanIndex = 1; FanIndex + 1 < static_cast<uint32>(ClippedVertices.size()); ++FanIndex)
			{
				DecalMeshData.Indices.push_back(BaseVertexIndex);
				DecalMeshData.Indices.push_back(BaseVertexIndex + FanIndex);
				DecalMeshData.Indices.push_back(BaseVertexIndex + FanIndex + 1);
			}
		}
	}

	if (GEngine && !DecalMeshData.Vertices.empty())
	{
		ID3D11Device* Device = GEngine->GetRenderer().GetFD3DDevice().GetDevice();
		if (Device)
		{
			MeshBuffer.Create(Device, DecalMeshData);
		}
	}

	UE_LOG("MeshDecal Result: Vertices=%d Indices=%d", static_cast<int>(DecalMeshData.Vertices.size()), static_cast<int>(DecalMeshData.Indices.size()));
	MarkProxyDirty(EDirtyFlag::Mesh);
}

void UMeshDecalComponent::ClipByDecalLocalOBB(TArray<FVertexPNCT>& OutClippedVertices, const FVertexPNCT& P0, const FVertexPNCT& P1, const FVertexPNCT& P2)
{
	TArray<FVertexPNCT> InputVertices = { P0, P1, P2 };
	TArray<FVertexPNCT> TempVertices;

	auto ClipPolygonByPlane = [&](float HExtent, uint32 AxisType, bool Larger)
		{
			if (InputVertices.empty())
			{
				return;
			}

			TempVertices.clear();
			const int32 VertexCount = static_cast<int32>(InputVertices.size());
			for (int32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
			{
				const FVertexPNCT& Start = InputVertices[VertexIndex];
				const FVertexPNCT& End = InputVertices[(VertexIndex + 1) % VertexCount];
				ClipByAxis(HExtent, AxisType, Start, End, Larger, TempVertices);
			}

			InputVertices = TempVertices;
		};

	ClipPolygonByPlane(-HalfExtent.X, 0, true);
	ClipPolygonByPlane(HalfExtent.X, 0, false);

	ClipPolygonByPlane(-HalfExtent.Y, 1, true);
	ClipPolygonByPlane(HalfExtent.Y, 1, false);

	ClipPolygonByPlane(-HalfExtent.Z, 2, true);
	ClipPolygonByPlane(HalfExtent.Z, 2, false);

	OutClippedVertices = InputVertices;
}

void UMeshDecalComponent::ClipByAxis(float HExtent, uint32 AxisType,
	const FVertexPNCT& Start, const FVertexPNCT& End,
	bool Larger, TArray<FVertexPNCT>& OutClippedVertices)
{
	float AxisValueStart = (AxisType == 0) ? Start.Position.X : (AxisType == 1) ? Start.Position.Y : Start.Position.Z;
	float AxisValueEnd = (AxisType == 0) ? End.Position.X : (AxisType == 1) ? End.Position.Y : End.Position.Z;

	bool bStartInside = Larger ? (AxisValueStart >= HExtent) : (AxisValueStart <= HExtent);
	bool bEndInside = Larger ? (AxisValueEnd >= HExtent) : (AxisValueEnd <= HExtent);

	auto Intersect = [&]() -> FVertexPNCT
		{
			float Denom = AxisValueEnd - AxisValueStart;
			if (Denom == 0.0f)
			{
				return Start;
			}

			float t = (HExtent - AxisValueStart) / Denom;

			FVertexPNCT Intersected = {};
			Intersected.Normal = (Start.Normal + (End.Normal - Start.Normal) * t).Normalized();
			Intersected.Position = Start.Position + (End.Position - Start.Position) * t;
			Intersected.Position += Intersected.Normal * MeshDecalSurfaceOffset;
			Intersected.Color = Start.Color + (End.Color - Start.Color) * t;
			Intersected.UV = ComputeDecalProjectedUV(Intersected.Position - Intersected.Normal * MeshDecalSurfaceOffset, HalfExtent);
			return Intersected;
		};

	// 안 -> 안 : end 추가
	if (bStartInside && bEndInside)
	{
		OutClippedVertices.push_back(End);
	}
	// 안 -> 밖 : 교차점만 추가
	else if (bStartInside && !bEndInside)
	{
		OutClippedVertices.push_back(Intersect());
	}
	// 밖 -> 안 : 교차점 + end 추가
	else if (!bStartInside && bEndInside)
	{
		OutClippedVertices.push_back(Intersect());
		OutClippedVertices.push_back(End);
	}
	// 밖 -> 밖 : 아무것도 안함
}
