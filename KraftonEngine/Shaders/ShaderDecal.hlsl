#include "Common/Functions.hlsl"
#include "Common/VertexLayouts.hlsl"

Texture2D SceneDepth : register(t0);        // Z-Buffer (SRV)
Texture2D DecalAlbedo : register(t1);       // Decal 텍스처
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);

// Decal OBB Local -> Screen MVP 변환
PS_Input_PosW VS(VS_Input_PC input)
{
    PS_Input_PosW output;
    output.position = ApplyMVP(input.position);
    output.positionW = output.position;
    return output;
}

// NDC → World Position 복원
float3 ReconstructWorldPos(float2 screenUV, float depth)
{
    // Screen UV (0~1) → NDC (-1~1)
    float2 ndc;
    ndc.x = screenUV.x * 2.0f - 1.0f;
    ndc.y = -(screenUV.y * 2.0f - 1.0f); // Y 반전 

    // NDC → World
    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos = mul(clipPos, InvViewProj);
    worldPos /= worldPos.w;

    return worldPos.xyz;
}

// Z-Buffer를 이용해 Screen -> Decal Local로 역변환
float4 PS(PS_Input_PosW input) : SV_TARGET
{
    // 1. 현재 픽셀의 Screen UV 계산
    //    ClipPosW.xy / ClipPosW.w → NDC, 이를 0~1 UV로
    float2 screenUV;
    screenUV.x = (input.positionW.x / input.positionW.w) * 0.5f + 0.5f;
    screenUV.y = (input.positionW.y / input.positionW.w) * -0.5f + 0.5f;
   
    // 2. Z-Buffer에서 Depth 샘플링
    float sceneDepth = SceneDepth.Sample(PointSampler, screenUV).r;

    // 3. Depth가 1.0이면 Sky/빈 공간 → 데칼 없음
    clip(sceneDepth < 1.0f ? 1 : -1);
    
    // 4. Screen UV + Depth → World Position 복원
    float3 worldPos = ReconstructWorldPos(screenUV, sceneDepth);

    // 5. World → Decal Local 좌표 변환
    float4 decalLocal = mul(float4(worldPos, 1.0f), WorldToDecal);
    
    // 6. Decal OBB 범위 체크 (-1 ~ 1)
    //    범위 밖이면 clip으로 픽셀 버림
    float3 absLocal = abs(decalLocal.xyz);
    clip(all(absLocal <= 0.5f) ? 1 : -1);

    // 7. Z-up, X축 Projection
    //    X축으로 Projection: YZ 평면이 텍스처 UV
    //    decalLocal.y → U, decalLocal.z → V
    //    로컬 좌표 -1~1 → UV 0~1
    float2 decalUV;
    decalUV.x = decalLocal.y * 1.0f + 0.5f; // Y → U
    decalUV.y = decalLocal.z * -1.0f + 0.5f; // Z → V (Z-up이므로 위가 0)
    
    // 8. Decal 텍스처 샘플링
    float4 decalColor = DecalAlbedo.Sample(LinearSampler, decalUV);
    
    // ============================================================
    // Fade In / Out
    // TODO: Offset들 Editor에서 조절 가능하게
    // ============================================================
    // 9. Edge Fade (Decal Cube의 가장자리에서 투명하게)
    const float fadeStart = 0.4f; // 튜닝 가능
    const float fadeEnd = 0.5f;
    
    float fadeX = 1.0f - smoothstep(fadeStart, fadeEnd, absLocal.x);
    float fadeY = 1.0f - smoothstep(fadeStart, fadeEnd, absLocal.y);
    float fadeZ = 1.0f - smoothstep(fadeStart, fadeEnd, absLocal.z);
    float edgeFade = fadeX * fadeY * fadeZ;
    
    // 10. Spot Fade (중앙은 선명, 가장자리는 투명)
    float2 uvCenter = decalUV - 0.5f; 
    float dist = length(uvCenter); 

    float spotFade = 1.0f - smoothstep(FadeInner, FadeOuter, dist);
    //spotFade = pow(spotFade, 2.0f);
   
    // 11. 최종 알파에 적용
    decalColor.a *= spotFade;
    //decalColor.a *= edgeFade * spotFade;

    // 12. Alpha 기반 클리핑 (선택)
    clip(decalColor.a - 0.01f);
    
    return decalColor;
}