R""(
//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 view;
    float4x4 projection;
};
cbuffer SceneConstantBuffer : register(b1)
{
    float4x4 world;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 color : TEXCOORD0;
};

PSInput VSMain(float4 position : POSITION, float2 color : TEXCOORD0)
{
    PSInput result;

    result.position = mul(projection, mul(view, mul(world, position)));
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color, 0, 1);
}
)""