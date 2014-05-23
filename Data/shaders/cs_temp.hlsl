#include "math.hlsl"


cbuffer g_Camera : register (b0)
{
	float4 g_CamPos;
	float4 g_CamLeft;
	float4 g_CamDir;
	float4 g_CamUp;
};

cbuffer g_OutputInfo : register (b1)
{
	uint2 g_OutResolution;
	float g_AspectRatio;
	float g_Some4ByteShit;
	float g_time;
};


SamplerState g_PointSampler : register (s0);



//render target
RWStructuredBuffer<float4> g_OutBuff : register( u0 );



#define MAT_DIFFUSE (1<<0)
#define MAT_REFLECTION (1<<1)

struct Material
{
	uint flags;
	float3 diffuse_color;
	float3 reflection_color;
};


struct Vertex
{
	float3 position;
	float3 normal;
};

struct Triangle 
{ 
	uint i0, i1, i2;
	uint matIndex;
};

StructuredBuffer<Vertex> g_Verticies;
StructuredBuffer<Triangle> g_Triangles;
StructuredBuffer<Material> g_Materials;


struct ShadingData
{
	float3 Pos;
	float3 Normal;
	//float3 Color;

	Material mat;
};

struct OmniLight
{
	float3 Pos;
	float3 Color;
};


/****THREAD DATA*******************************************************/

struct StackEl
{
	uint ret; //1
	float3 retColor; //3
	Ray ray; //9
	ShadingData sd; //16
};

/*
RWStructuredBuffer<StackEl> g_StackBuffer : register( u1 );
//RWStructuredBuffer<uint> g_StackPointerBuffer : register( u2 );

StackEl StackGet(uint threadIndex, uint at)
{
	return g_StackBuffer[STACK_DEPTH * threadIndex + at];
}

void StackSet(uint threadIndex, uint at, StackEl el)
{
	g_StackBuffer[STACK_DEPTH * threadIndex + at] = el;
}
*/

/*
struct ThreadData
{
	uint stackPtr;
	float4 globals[16]; // 64 global floats = 16 float4 (for dimensions etc. )
	float4 data[64][16]; // 64 calls ( 1 call can have 64 floats = 16 float4 )
};

// if THREAD_COUNT == 512 then sizeof(g_ThreadDataBuffer) = ~128MB

RWStructuredBuffer<ThreadData> g_ThreadDataBuffer : register( u1 );
*/


/**********************************************************************/


struct TreeNode
{
	Box		box;
	uint	leaf[2];
	uint	count;
	int		successJump;
	int		failureJump;
};

StructuredBuffer<TreeNode> g_TreeNodes;

IntersectInfo IntersectRayTriangleTree(Ray ray, uint triFilter)
{
	int			currentNodeIndex = 0;
	TreeNode	currentNode;

	Triangle	meshTriangle;
	Tri			tri;
	Vertex		vert[3];
	float		tmp_t;

	uint triIndex;
	IntersectInfo	info;
	info.t			= 1e+20;

	float2 uv;

	while(currentNodeIndex >= 0)
	{
		currentNode = g_TreeNodes[currentNodeIndex];

		if(IntersectRayBox(ray, currentNode.box) == 1)
		{
			currentNodeIndex = currentNode.successJump;

			if(currentNode.count > 0)
			{
				triIndex = currentNode.leaf[0];

				if (triIndex != triFilter)
				{
					meshTriangle = g_Triangles[ triIndex ];
					vert[0] = g_Verticies[meshTriangle.i0];
					vert[1] = g_Verticies[meshTriangle.i1];
					vert[2] = g_Verticies[meshTriangle.i2];

					tri.V0 = vert[0].position;
					tri.V1 = vert[1].position;
					tri.V2 = vert[2].position;

					tmp_t = IntersectRayTri(ray, tri, uv);

					if ((tmp_t > 0) && (tmp_t < info.t))
					{
						info.t = tmp_t;
						info.triIndex = triIndex;
						info.uv = uv;
					}
				}

				if(currentNode.count > 1)
				{
					triIndex = currentNode.leaf[1];
					if (triIndex != triFilter)
					{
						meshTriangle = g_Triangles[ triIndex ];
						vert[0] = g_Verticies[meshTriangle.i0];
						vert[1] = g_Verticies[meshTriangle.i1];
						vert[2] = g_Verticies[meshTriangle.i2];

						tri.V0 = vert[0].position;
						tri.V1 = vert[1].position;
						tri.V2 = vert[2].position;

						tmp_t = IntersectRayTri(ray, tri, uv);

						if ((tmp_t > 0) && (tmp_t < info.t))
						{
							info.t = tmp_t;
							info.triIndex = triIndex;
							info.uv = uv;
						}		
					}
				}
			}
		}
		else currentNodeIndex = currentNode.failureJump;
	}
	return info;
}

float RaytraceShadow(Ray ray, float t_max)
{
	IntersectInfo info = IntersectRayTriangleTree(ray, 0xFFFFFFFF);
	if (info.t >= t_max)
		return 1.0f;

	return 0.0f;
}

float3 CalcOmniLight(OmniLight light, ShadingData sd, uint cast_shadow)
{
	float3 LightVec = light.Pos - sd.Pos;
	float dist = length(LightVec);
	LightVec /= dist;

	float NdotL = dot(sd.Normal, LightVec);

	if (NdotL>0)
	{
		Ray shadowRay;
		shadowRay.Origin = sd.Pos + 0.0001f * sd.Normal;
		shadowRay.Dir = LightVec;
		shadowRay.invDir = 1 / shadowRay.Dir;

		float shadow = 1;
		if(cast_shadow)
		shadow *= RaytraceShadow(shadowRay, dist + 0.001f);
		return shadow * light.Color * max(0.0f, NdotL) / (dist*dist);
	}

	return float3(0, 0, 0);
}

#define backgroundColor (float3(0.2, 0.3, 0.4))


ShadingData EvaluateShadingData(Ray ray, IntersectInfo info)
{
	ShadingData sd;

	sd.Pos = ray.Origin + ray.Dir * info.t;


	Vertex vert[3];
	Triangle meshTriangle = g_Triangles[info.triIndex];

	vert[0] = g_Verticies[meshTriangle.i0];
	vert[1] = g_Verticies[meshTriangle.i1];
	vert[2] = g_Verticies[meshTriangle.i2];

	float3 normals[3];
	normals[0] = vert[0].normal;
	normals[1] = vert[1].normal;
	normals[2] = vert[2].normal;
	sd.Normal = vert[1].normal * info.uv.x +
				vert[2].normal * info.uv.y +
				vert[0].normal * (1 - info.uv.x - info.uv.y);


	sd.mat = g_Materials[meshTriangle.matIndex];

	return sd;
}

groupshared StackEl g_stack[THREADS_X*THREADS_Y][4];

float3 Raytrace(Ray ray, uint threadIndex, uint3 threadIDInGroup)
{
	uint index = threadIDInGroup.y*THREADS_X+threadIDInGroup.x;

	StackEl el = (StackEl)0;
	el.ret = 0;
	el.ray = ray;
	el.retColor = float3(0, 0, 0);

	int stackPtr = 0;
	g_stack[index][0] = el;
	g_stack[index][1] = el;
	g_stack[index][2] = el;
	g_stack[index][3] = el;

	//define some light
	OmniLight light;
	float time = g_time*0.25f;
	light.Pos = float3(cos(time)*1, 3 + sin(time) * 1.5f, sin(time)*1);
	light.Color = float3(125, 100, 50);

	IntersectInfo info;

	[loop] while(stackPtr < 3)
	{
		el = g_stack[index][stackPtr];
		
		info = IntersectRayTriangleTree(el.ray, 0xFFFFFFFF);

		if (info.t > 1e+19)
		{
			el.retColor += backgroundColor;
			return el.retColor;
		}

		el.sd = EvaluateShadingData(el.ray, info);
		el.ret = 1;

		if (el.sd.mat.flags == MAT_DIFFUSE)
		{
			el.retColor += el.sd.mat.diffuse_color * (CalcOmniLight(light, el.sd, 1));
		}

		stackPtr++;
		continue;

		/*if (el.sd.mat.flags & MAT_REFLECTION )
		{
			if(el.ret<2)
			{
				if(el.ret)el.retColor = el.sd.mat.reflection_color * g_stack[index][stackPtr+1].retColor;
				el.ray.Origin = el.sd.Pos + el.sd.Normal*0.001f;
				el.ray.Dir = reflect(el.ray.Dir, el.sd.Normal);
				el.ray.invDir = 1 / el.ray.Dir;
				g_stack[index][stackPtr+1] = el;
				stackPtr+=2;
				el.ret++;
			}
		}*/
	}

	return el.retColor;
}



float3 ToneMapping(float3 color, float exposure)
{
	return float3(1,1,1) - exp( - color * exposure);
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID,
		  uint3 groupID : SV_GroupID,
		  uint3 dispatchThreadID : SV_DispatchThreadID,
		  uint groupIndex : SV_GroupIndex)
{
	if (any(dispatchThreadID.xy >= g_OutResolution))
		return;

	uint threadIndex = threadIDInGroup.x + THREADS_X * threadIDInGroup.y;
	
	//calc screen coordinates (0..1)
	float2 coords = (float2)dispatchThreadID.xy / float2(g_OutResolution.xy);
	coords -= float2(0.5, 0.5);
	coords *= 2.0f;
	coords.x *= g_AspectRatio;

	float3 up = cross(g_CamDir.xyz, g_CamLeft.xyz);
	up = normalize(up);

	// generate ray
	Ray ray;
	ray.Origin = g_CamPos.xyz;
	ray.Dir = g_CamDir.xyz + coords.x*g_CamLeft.xyz + coords.y*up;
	ray.Dir = normalize(ray.Dir);
	ray.invDir = 1 / ray.Dir;

	float3 color = Raytrace(ray, threadIndex, threadIDInGroup);
	


	color = ToneMapping(color, 1);
	g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = color.xyzz;
}