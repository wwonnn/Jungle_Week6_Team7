#include "Common/VertexLayouts.hlsl"
#include "Common/ConstantBuffers.hlsl"
#include "Common/Functions.hlsl"

Texture2D SceneDepth : register(t0);
SamplerState PointSampler : register(s0);
// ─── Vertex Shader ───
// Cube 정점에 MVP 적용, clip-space 좌표를 PS에 전달
float3 ReconstructWorldPos(float2 screenUV, float depth)
{
    float2 ndc;
    ndc.x = screenUV.x * 2.0 - 1.0;
    ndc.y = -(screenUV.y * 2.0 - 1.0); // DX는 Y 반전
    
    float4 clipPos = float4(ndc, depth, 1.0);
    float4 worldPos = mul(clipPos, InvViewProj);
    worldPos /= worldPos.w; // perspective division
    return worldPos.xyz;
}
PS_Input_PosW VS(VS_Input_PC input)
{
    PS_Input_PosW output;
    output.position = ApplyMVP(input.position);
    output.positionW = output.position;
    return output;
}
float4 PS(PS_Input_PosW input) : SV_TARGET
{
    //Screen UV 
    float2 screenUV;
    screenUV.x = (input.positionW.x / input.positionW.w) * 0.5 + 0.5;
    screenUV.y = (input.positionW.y / input.positionW.w) * -0.5 + 0.5;
    
    //Scene Depth
    float sceneDepth = SceneDepth.Sample(PointSampler, screenUV).r;
    if (sceneDepth >= 1.0)
        discard;
    // world pos 
    float3 worldPos = ReconstructWorldPos(screenUV, sceneDepth);
    
    // Local space convert, Local X = 투영 방향 (Forward) ,Local Y = 좌우, Local Z = 상하
    float3 localPos = mul(float4(worldPos, 1.0), WorldToLight).xyz;

    // projection Depth ( X direction
    float projDepth = localPos.x;
    if (projDepth<=0.0)
        discard;
   
    // distance attenuation (Inverse Square Falloff
    float distRatio = projDepth / AttenuationRadius;
    if (distRatio > 1.0)
        discard;
    // UE style attenuation (1 - (d/r)²)²
    float distAttenuation = saturate(1.0 - distRatio * distRatio);
    distAttenuation *= distAttenuation;
    
    // Cone angle check
    // Localpos Lomalize -> light dir and angle
    float3 dirToPixel = normalize(localPos);
    float cosAngle = dirToPixel.x; // dot(dir, forward) = dir.x (forwar = +X)
    
    if (cosAngle - OuterConeAngleCos)
        discard;
    // inter to outer attenuation
    float spotFalloff = saturate((cosAngle - OuterConeAngleCos) /
        max(InnerConeAngleCos - OuterConeAngleCos, 0.001));
    // final light contribute
    float3 lightContribution = LightColor * Intensity * distAttenuation * spotFalloff;

    float luminance = dot(lightContribution, float3(0.299, 0.587, 0.114));
    if (luminance < 0.001)
        discard;


    return float4(lightContribution, 1.0);

}