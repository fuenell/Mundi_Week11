// --- GPU 스키닝 매크로 ---
#ifndef USE_GPU_SKINNING
#define USE_GPU_SKINNING 0
#endif

#define GPU_SKINNING_BUFFER_REGISTER t12
#define GPU_SKINNING_NORMAL_BUFFER_REGISTER t13

#if USE_GPU_SKINNING
typedef row_major float4x4 FRowMajorMatrix;
StructuredBuffer<FRowMajorMatrix> g_SkinningMatrices : register(GPU_SKINNING_BUFFER_REGISTER);
#endif

// b0: ModelBuffer (VS) - ModelBufferType과 정확히 일치 (128 bytes)
cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix; // 64 bytes
    row_major float4x4 WorldInverseTranspose; // 64 bytes - 올바른 노멀 변환을 위함
};

// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;   // 0행 광원의 월드 좌표 + 스포트라이트 반경
    row_major float4x4 InverseProjectionMatrix; 
};

// --- 셰이더 입출력 구조체 ---
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
    float4 Tangent : TANGENT0;
    float4 Color : COLOR;
#if USE_GPU_SKINNING
    uint4 BoneIndices : BLENDINDICES0;
    float4 BoneWeights : BLENDWEIGHT0;
#endif
};

// 출력은 오직 클립 공간 위치만 필요
struct VS_OUT
{
    float4 Position : SV_Position;
    float3 WorldPosition : TEXCOORD0;
};

VS_OUT mainVS(VS_INPUT Input)
{
    VS_OUT Output = (VS_OUT) 0;

    float4 LocalPosition = float4(Input.Position, 1.0f);

#if USE_GPU_SKINNING
    float4 SkinnedPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float BoneWeightSum = 0.0f;

    [unroll]
    for (uint Idx = 0; Idx < 4; ++Idx)
    {
        const uint BoneIndex = Input.BoneIndices[Idx];
        const float BoneWeight = Input.BoneWeights[Idx];

        if (BoneWeight > 0.0f)
        {
            const float4x4 SkinMatrix = g_SkinningMatrices[BoneIndex];
            SkinnedPosition += mul(LocalPosition, SkinMatrix) * BoneWeight;
            BoneWeightSum += BoneWeight;
        }
    }

    if (BoneWeightSum > 0.0f)
    {
        LocalPosition = SkinnedPosition;
    }
#endif

    // 모델 좌표 -> 월드 좌표 -> 뷰 좌표 -> 클립 좌표
    float4 WorldPos = mul(LocalPosition, WorldMatrix);
    float4 ViewPos = mul(WorldPos, ViewMatrix);
    Output.Position = mul(ViewPos, ProjectionMatrix);
    Output.WorldPosition = WorldPos.xyz;
    
    return Output;
}
