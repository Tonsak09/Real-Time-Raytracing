
// === Defines ===

#define PI 3.141592654f

// === Structs ===

// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition	: POSITION;
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
    float3 tangent			: TANGENT;
};
static const uint VertexSizeInBytes = 11 * 4; // 11 floats total per vertex * 4 bytes each


// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible
struct RayPayload
{
	float3 color;
	uint recursionDepth;
	uint rayPerPixelIndex;
};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes struct
// for triangle attributes, so no need to define our own.  It contains a single float2.



// === Constant buffers ===

cbuffer SceneData : register(b0)
{
	matrix inverseViewProjection;
	float3 cameraPosition;
	float pad0;
};


// Ensure this matches C++ buffer struct define!
#define MAX_INSTANCES_PER_BLAS 100
cbuffer ObjectData : register(b1)
{
	float4 entityColor[MAX_INSTANCES_PER_BLAS];
	float4 lightHue[MAX_INSTANCES_PER_BLAS];
};


// === Resources ===

// Output UAV 
RWTexture2D<float4> OutputColor				: register(u0);

// The actual scene we want to trace through (a TLAS)
RaytracingAccelerationStructure SceneTLAS	: register(t0);

// Geometry buffers
ByteAddressBuffer IndexBuffer        		: register(t1);
ByteAddressBuffer VertexBuffer				: register(t2);


static const float indexOfRefraction = 1.5;

// === Helpers ===

// Loads the indices of the specified triangle from the index buffer
uint3 LoadIndices(uint triangleIndex)
{
	// What is the start index of this triangle's indices?
	uint indicesStart = triangleIndex * 3;

	// Adjust by the byte size before loading
	return IndexBuffer.Load3(indicesStart * 4); // 4 bytes per index
}

// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float3 barycentricData)
{
	// Grab the indices
	uint3 indices = LoadIndices(triangleIndex);

	// Set up the final vertex
	Vertex vert;
	vert.localPosition = float3(0, 0, 0);
	vert.uv = float2(0, 0);
	vert.normal = float3(0, 0, 0);
	vert.tangent = float3(0, 0, 0);

	// Loop through the barycentric data and interpolate
	for (uint i = 0; i < 3; i++)
	{
		// Get the index of the first piece of data for this vertex
		uint dataIndex = indices[i] * VertexSizeInBytes;

		// Grab the position and offset
		vert.localPosition += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// UV
		vert.uv += asfloat(VertexBuffer.Load2(dataIndex)) * barycentricData[i];
		dataIndex += 2 * 4; // 2 floats * 4 bytes per float

		// Normal
		vert.normal += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// Tangent (no offset at the end, since we start over after looping)
		vert.tangent += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
	}

	// Final interpolated vertex data is ready
	return vert;
}

// Gets a random value in 1 dimension
float rand(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// Gets a random value in 2 dimension
float2 rand2(float2 uv)
{
	float x = rand(uv);
	float y = sqrt(1 - x * x);
	return float2(x, y);
}

// Gets a random value in 3 dimension
float3 rand3(float2 uv)
{
	return float3(
		rand2(uv),
		rand(uv.yx));
}

// Gets a random vector within a unit circle 
float3 RandomVector(float u0, float u1)
{
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * PI * u1;
	float x = b * cos(phi);
	float y = b * sin(phi);
	float z = a;
	return float3(x, y, z);
}

// Random vector within a hemisphere 
float3 RandomCosineWeightedHemisphere(float u0, float u1, float3 unitNormal)
{
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * PI * u1;
	float x = unitNormal.x + b * cos(phi);
	float y = unitNormal.y + b * sin(phi);
	float z = unitNormal.z + a;
	return float3(x, y, z);
}

// Calculates an origin and direction from the camera fpr specific pixel indices
void CalcRayFromCamera(float2 rayIndices, out float3 origin, out float3 direction)
{
	// Offset to the middle of the pixel
	float2 pxy = float2(-0.5 + rand(rayIndices), -0.5 + rand(rayIndices)); // Rand offset 
	float2 pixel = pxy + rayIndices + 0.5f;

	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(inverseViewProjection, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the outputs
	origin = cameraPosition.xyz;
	direction = normalize(worldPos.xyz - origin);
}

float3 reflect(float3 v, float3 n)
{
	return v - 2.0 * dot(v, n) * n;
}

float3 refract(float3 uv, float3 normal, float etaiOverEtat)
{
	float cosTheta = min(dot(-uv, normal), 1.0);
	float3 rOutPerp = etaiOverEtat * (uv * cosTheta * normal);
	float3 rOutParallel = -sqrt(abs(1.0 - length(rOutPerp))) * normal;
	return rOutPerp + rOutParallel;
}

// === Shaders ===

// Ray generation shader - Launched once for each ray we want to generate
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
	// Get the ray indices
	uint2 rayIndices = DispatchRaysIndex().xy;

	// Calculate the ray data
	float3 rayOrigin;
	float3 rayDirection;

	int raysPerPixel = 5;
	float3 totalColor = float3(0, 0, 0);
	for (int i = 0; i < raysPerPixel; i++)
	{
		CalcRayFromCamera(rayIndices, rayOrigin, rayDirection);

		// Set up final ray description
		RayDesc ray;
		ray.Origin = rayOrigin;
		ray.Direction = rayDirection;
		ray.TMin = 0.0001f;
		ray.TMax = 1000.0f;

		// Set up the payload for the ray
		// This initializes the struct to all zeros
		RayPayload payload = (RayPayload)0;

		// Perform the ray trace for this ray
		TraceRay(
			SceneTLAS,
			RAY_FLAG_NONE,
			0xFF,
			0,
			0,
			0,
			ray,
			payload);

		totalColor += payload.color;
	}


	// Set the final color of the buffer (gamma corrected)
	//OutputColor[rayIndices] = float4(pow(payload.color, 1.0f / 2.2f), 1);
	OutputColor[rayIndices] = float4(pow(totalColor / raysPerPixel, 1.0f / 2.2f), 1);
}


// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
	// Hemispheric gradient
	float3 upColor = float3(0.3f, 0.5f, 0.95f);
	float3 downColor = float3(1, 1, 1);

	// Interpolate based on the direction of the ray
	float interpolation = dot(normalize(WorldRayDirection()), float3(0, 1, 0)) * 0.5f + 0.5f;

	if (payload.recursionDepth == 0)
	{
		payload.color = lerp(downColor, upColor, interpolation);
	}
	else
	{
		// Amient light 
		payload.color *= 0.1 * lerp(downColor, upColor, interpolation);
	}
}


// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	if (payload.recursionDepth >= 10)
	{
		payload.color = float3(0, 0, 0);
		return;
	}


	// Grab the index of the triangle we hit
	uint triangleIndex = PrimitiveIndex();

	// Calculate the barycentric data for vertex interpolation
	float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Get the interpolated vertex data
	Vertex interpolatedVert = InterpolateVertices(triangleIndex, barycentricData);

	// Get the data for this entity
	uint instanceID = InstanceID();

	/*if (isLightSource[instanceID] == true)
	{
		payload.color += entityColor[instanceID].rgb;
	}
	else
	{
		payload.color *= entityColor[instanceID].rgb;
	}*/

	payload.color += lightHue[instanceID].rgb;
	payload.color *= entityColor[instanceID].rgb;

	// Create another recurssive ray 
	float2 uv = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

	float2 rng = rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	float3 randBounce = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), interpolatedVert.normal);
	float3 refl = reflect(WorldRayDirection(), interpolatedVert.normal);
	float3 dir = normalize(lerp(refl, randBounce, entityColor[instanceID].a));
	

	RayDesc ray;
	ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + (dir * 0.1f);
	ray.Direction = dir;
	ray.TMin = 0.0001f;
	ray.TMax = 1000.0f;

	payload.recursionDepth++;
	TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,	// Mis shader index 
		ray,
		payload);
	
	// Call secondary shadow ray 
	//  -> Could influcde secondary payload
}
