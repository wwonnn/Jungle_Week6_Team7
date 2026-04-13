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
    
    // 6. Decal OBB 범위 체크 (-0.5 ~ 0.5)
    float3 absLocal = abs(decalLocal.xyz);
    clip(all(absLocal <= 0.5f) ? 1 : -1);

    // 7. UV 프로젝션 (YZ 평면 사용)
    float2 decalUV;
    decalUV.x = decalLocal.y * 1.0f + 0.5f;
    decalUV.y = decalLocal.z * -1.0f + 0.5f;
    
    // 8. Decal 텍스처 샘플링
    float4 decalColor = DecalAlbedo.Sample(LinearSampler, decalUV);
    
    // 9. 페이드 효과 (선택적 적용)
    if (bUseFade != 0)
    {
        // 깊이 축 감쇠 (박스의 끝부분에서 부드럽게 사라지게 함)
        const float fadeStart = 0.4f;
        const float fadeEnd = 0.5f;
        float depthFade = 1.0f - smoothstep(fadeStart, fadeEnd, absLocal.x);
        
        // 2D 원형 감쇠
        float2 uvCenter = decalUV - 0.5f; 
        float dist = length(uvCenter); 
        float spotFade = 1.0f - smoothstep(FadeInner, FadeOuter, dist);

        decalColor.a *= spotFade * depthFade;
    }

    // 10. Alpha 기반 클리핑
    clip(decalColor.a - 0.01f);
    
    return decalColor;
}