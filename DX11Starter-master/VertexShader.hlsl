#include "ShaderInclude.hlsli"

cbuffer matricies : register(b0)
{
    matrix world;
    matrix worldInvTranspose;
    matrix view;
    matrix proj;
}


// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float3 normal			: NORMAL;     
	float3 tangent			: TANGENT;    
    float2 uv				: TEXCOORD;
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;
	//output.screenPosition = float4(input.localPosition, 1.0f);
	
    matrix mvp = mul(proj, mul(view, world));
    output.screenPosition = mul(mvp, float4(input.localPosition, 1.0f));
    output.normal = mul((float3x3) worldInvTranspose, input.normal); // Perfect
    output.tangent = mul((float3x3) world, input.tangent);
    output.uv = input.uv;
	
    output.worldPosition = mul(world, float4(input.localPosition, 1.0f)).xyz;
	
	return output;
}