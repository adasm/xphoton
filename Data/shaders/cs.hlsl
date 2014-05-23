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
	uint2 g_InitialSeed;
	uint2 g_OutResolution;
	float g_AspectRatio;
	float g_numSample;
	float2 sth;
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
	float  glossy_factor; 
	float3 emission_color;
	
};


struct Vertex
{
	float3 position;
	float3 normal;
	float3 tangent;
};

struct Triangle 
{ 
	uint i0, i1, i2;
	uint matIndex;
};

StructuredBuffer<Vertex> g_Verticies : register (t0);
StructuredBuffer<Triangle> g_Triangles : register (t1);
StructuredBuffer<Material> g_Materials : register (t2);
StructuredBuffer<float4> g_tex3d: register (t7);


struct ShadingData
{
	float3 Pos;
	float3 Normal;
	float3 Tangent;
	float3 Color;

	Material mat;
};

struct Light
{
	float3 Pos;
	float3 Color;
	float Radius;
};

StructuredBuffer<Light> g_LightBuffer : register (t4);

/****THREAD DATA*******************************************************/

struct StackEl
{
	uint retAdress; //1
	float3 retColor; //3

	Ray ray; //9
	IntersectInfo info; //4
	ShadingData sd; //16
};

/**********************************************************************/


struct TreeNode
{
	Box		box;
	uint	leaf[2];
	uint	count;
	int		successJump;
	int		failureJump;
};

StructuredBuffer<TreeNode> g_TreeNodes : register (t3);
StructuredBuffer<TreeNode> g_LightTreeNodes : register (t5);

StructuredBuffer<uint2> g_RandSeed : register (t6);



IntersectInfo IntersectRayTriangleTreeOrg(Ray ray, uint triFilter)
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

IntersectInfo IntersectRayTriangleTreeOrgDistMax(Ray ray, uint triFilter, float distMax)
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

		float3 boxCenter = (currentNode.box.Min+currentNode.box.Max)/2;
		float radius = length(currentNode.box.Max-currentNode.box.Min)/2;
		
		if(length(boxCenter-ray.Origin)>radius+distMax)
		{
			currentNodeIndex = currentNode.failureJump;
			continue;
		}
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

/*IntersectInfo IntersectRayTriangleTree(Ray ray, uint triFilter)
{
	IntersectInfo	info;
	float step	= 200.0f, step2 = 1.5f*step;
	info = IntersectRayTriangleTreeOrgDistMax(ray, triFilter, step2);
	while(info.t<step2)
	{
		ray.Origin = normalize(ray.Dir)*step;
		info = IntersectRayTriangleTreeOrgDistMax(ray, triFilter, step2);
	}
	return IntersectRayTriangleTreeOrg(ray,;
}*/

float RaytraceShadow(Ray ray, float t_max)
{
	IntersectInfo info = IntersectRayTriangleTreeOrg(ray, 0xFFFFFFFF);
	if (info.t >= t_max)
		return 1.0f;

	return 0.0f;
}

float3 CalcOmniLight(Light light, ShadingData sd, uint cast_shadow)
{
	float3 LightVec = light.Pos + 1.0f*random_vec() - sd.Pos;
	float dist = length(LightVec);
	LightVec /= dist;

	float NdotL = dot(sd.Normal, LightVec);

	if (NdotL>0 && dist<light.Radius)
	{
		Ray shadowRay;
		shadowRay.Origin = sd.Pos + 0.0001f * sd.Normal;
		shadowRay.Dir = LightVec;
		shadowRay.invDir = 1 / shadowRay.Dir;

		float shadow = 1;
		if(cast_shadow)
		shadow *= RaytraceShadow(shadowRay, dist + 0.001f);
		
		float falloff = 1.0f - dist/light.Radius;
		shadow *= falloff*falloff*falloff;

		return shadow * light.Color * max(0.0f, NdotL) / (dist*dist);
	}

	return float3(0, 0, 0);
}

float3 CalculateIllumination(ShadingData sd, uint cast_shadow)
{
	float3 result = float3(0, 0, 0);
	int			currentNodeIndex = 0;
	TreeNode	currentNode;

	uint lightIndex;
	Light light;

	while(currentNodeIndex >= 0)
	{
		currentNode = g_LightTreeNodes[currentNodeIndex];

		if (IntersectPointBox(sd.Pos, currentNode.box))
		{
			currentNodeIndex = currentNode.successJump;

			if(currentNode.count > 0)
			{
				lightIndex = currentNode.leaf[0];
				light = g_LightBuffer[lightIndex];
				result += CalcOmniLight(light, sd, cast_shadow);

				if(currentNode.count > 1)
				{
					lightIndex = currentNode.leaf[1];
					light = g_LightBuffer[lightIndex];
					result += CalcOmniLight(light, sd, cast_shadow);
				}
			}
		}
		else currentNodeIndex = currentNode.failureJump;
	}
	return result;
}



#define backgroundColor (float3(0.75f, 0.85f, 1.0f))


ShadingData EvaluateShadingData(Ray ray, IntersectInfo info)
{
	ShadingData sd;

	sd.Pos = ray.Origin + ray.Dir * info.t;


	Vertex vert[3];
	Triangle meshTriangle = g_Triangles[info.triIndex];

	vert[0] = g_Verticies[meshTriangle.i0];
	vert[1] = g_Verticies[meshTriangle.i1];
	vert[2] = g_Verticies[meshTriangle.i2];

	info.uv.x = abs(info.uv.x);
	info.uv.y = abs(info.uv.y);

	float3 normals[3];
	normals[0] = vert[0].normal;
	normals[1] = vert[1].normal;
	normals[2] = vert[2].normal;
	sd.Normal = vert[1].normal * info.uv.x +
				vert[2].normal * info.uv.y +
				vert[0].normal * (1 - info.uv.x - info.uv.y);
	sd.Normal = normalize(sd.Normal);


	float3 tangents[3];
	tangents[0] = vert[0].tangent;
	tangents[1] = vert[1].tangent;
	tangents[2] = vert[2].tangent;
	sd.Tangent = vert[1].tangent * info.uv.x +
				 vert[2].tangent * info.uv.y +
				 vert[0].tangent * (1 - info.uv.x - info.uv.y);

	sd.mat = g_Materials[meshTriangle.matIndex];

	return sd;
}

struct ST
{
	float3 factor;
	float3 input_color;
	float3  emission;
};

//groupshared ST g_shared_stack[THREADS_X*THREADS_Y][STACK_DEPTH]; //diffuse_color, direct_illumination
//#define stack g_shared_stack[threadIndex]

float3 Raytrace(Ray ray, uint threadIndex, int max_bounces)
{
	StackEl el = (StackEl)0;
	el.retAdress = 0;
	el.ray = ray;
	el.retColor = float3(0, 0, 0);
	int bounce = 0;
	float3 oldRayDir;
	float nepsilon = 0.001f;
	ST stack[STACK_DEPTH];

	[loop] for(; bounce < max_bounces; bounce++)
	{
		el.info = IntersectRayTriangleTreeOrg(el.ray, 0xFFFFFFFF);

		if (el.info.t > 1e+19)
		{
			stack[bounce].emission = float3(0.3f, 0.4f, 0.5f);
			stack[bounce].factor = float3(1, 1, 1);
			stack[bounce].input_color = backgroundColor;
			bounce++;
			break;
		}

		el.sd = EvaluateShadingData(el.ray, el.info);

		if(el.sd.mat.flags == MAT_DIFFUSE)
		{
			stack[bounce].emission = el.sd.mat.emission_color + CalculateIllumination(el.sd, (g_numSample<1)?(0):(2));
			stack[bounce].factor = el.sd.mat.diffuse_color;		
			stack[bounce].input_color = (g_numSample<1)?(el.sd.mat.diffuse_color):(0);	
			float3x3 TBN = float3x3(el.sd.Tangent, cross(el.sd.Tangent, el.sd.Normal), el.sd.Normal);
			el.ray.Origin = el.sd.Pos + el.sd.Normal*nepsilon;
			el.ray.Dir = mul(randomVecOnHemisphere(), TBN);
			//el.ray.Dir = reflect(el.ray.Dir, el.sd.Normal);
			el.ray.invDir = 1 / el.ray.Dir;
		}
		else 
		{
				stack[bounce].emission = 0;
				stack[bounce].factor = el.sd.mat.reflection_color;
				stack[bounce].input_color = el.sd.mat.reflection_color;

				

				float glossy_factor = el.sd.mat.glossy_factor;
				bool into = (dot(el.sd.Normal, el.ray.Dir) < 0);
				if(glossy_factor>nepsilon) //reflection/glossy_reflection
				{
					float3x3 TBN = float3x3(el.sd.Tangent, cross(el.sd.Tangent, el.sd.Normal), el.sd.Normal);
					el.ray.Origin = el.sd.Pos + el.sd.Normal*nepsilon;
					el.ray.Dir = normalize(reflect(el.ray.Dir, el.sd.Normal)*(1-glossy_factor)+glossy_factor*mul(randomVecOnHemisphere(), TBN));
					el.ray.invDir = 1 / el.ray.Dir;
				}
				/*else
				{
					float into_f = (into)?(1):(-1);
					el.ray.Origin = el.sd.Pos + into_f*el.sd.Normal*nepsilon;
					el.ray.Dir = reflect(el.ray.Dir, into_f*el.sd.Normal);
					el.ray.invDir = 1 / el.ray.Dir;
				}*/
				else if(randFloat() < 0.85f || !into) // refraction(90%) + reflection (10%)
				{
					float into_f = (into)?(-1):(1);
					el.ray.Origin = el.sd.Pos + into_f*el.sd.Normal*nepsilon;
					float	n1 = 1.0f, n2 = 1.5f; //outside medium, inside medium
					float	n = (into)?(n1/n2):(n2/n1);
					float3	N = -into_f*el.sd.Normal;
					float cosI = dot(N, el.ray.Dir); //-
					float cosT2 = 1.0f - n * n * (1.0f - cosI * cosI);
					if (cosT2 >= 0.0f)
					{
						float3 T = (n * el.ray.Dir) - (n * cosI + sqrt( cosT2 )) * N; // + -
						el.ray.Dir = normalize(T);
						el.ray.invDir = 1 / el.ray.Dir;
					}
					else if(into)//reflection outside
					{
						el.ray.Origin = el.sd.Pos + el.sd.Normal*nepsilon;
						el.ray.Dir = normalize(reflect(el.ray.Dir, el.sd.Normal));
						el.ray.invDir = 1 / el.ray.Dir;
					}
					else//reflection inside
					{
						el.ray.Origin = el.sd.Pos - el.sd.Normal*nepsilon;
						el.ray.Dir = normalize(reflect(el.ray.Dir, -el.sd.Normal));
						el.ray.invDir = 1 / el.ray.Dir;
					}
				}
				else// refraction(90%) + reflection (10%)
				{
					el.ray.Origin = el.sd.Pos + el.sd.Normal*nepsilon;
					el.ray.Dir = reflect(el.ray.Dir, el.sd.Normal);
					el.ray.invDir = 1 / el.ray.Dir;
				}

				//stack[bounce].factor *= 1-dot(el.sd.Normal, el.ray.Dir);
		}
	}

	bounce--;
	[loop] while(bounce-->0)
		stack[bounce].input_color = stack[bounce+1].input_color * stack[bounce+1].factor + stack[bounce+1].emission;
	return stack[0].input_color * stack[0].factor + stack[0].emission;
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID,
		  uint3 groupID : SV_GroupID,
		  uint3 dispatchThreadID : SV_DispatchThreadID,
		  uint groupIndex : SV_GroupIndex)
{
	if(g_numSample < 1.0f)
		dispatchThreadID.xy *= 2;

	if (any(dispatchThreadID.xy >= g_OutResolution))
		return;
	seed = g_InitialSeed + g_RandSeed[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y];
	uint threadIndex = threadIDInGroup.x + THREADS_X * threadIDInGroup.y;

	float2 coords;
	Ray ray;

	float3 up = cross(g_CamDir.xyz, g_CamLeft.xyz);
	up = normalize(up);

	float w = 1.0f, w2 = 1.0f;
	float2 rnd_coords = (g_numSample >= 1.0f)?((g_numSample<10)?(w*randFloat2()-w/2):(w2*randFloat2()-w2/2)):(float2(0,0));
	coords = (float2)((float2)dispatchThreadID.xy + rnd_coords) / (float)(g_OutResolution.xy);
	coords -= float2(0.5, 0.5);
	coords *= 2;
	//coords.xy *= g_OutResolution.y/g_OutResolution.x * 2.0f;
	//coords.x *= 2.0f;//g_OutResolution.y/g_OutResolution.x;
	//coords.y *= g_OutResolution.y/g_OutResolution.x*2.0f;
	
	ray.Origin = g_CamPos.xyz + g_CamDir.xyz + coords.x*g_CamLeft.xyz + coords.y*up;
	//ray.Dir = g_CamDir.xyz + coords.x*g_CamLeft.xyz + coords.y*up;
	ray.Dir = ray.Origin - g_CamPos.xyz;
	ray.Dir = normalize(ray.Dir);
	ray.invDir = 1 / ray.Dir;
	
	
	

	uint numSamples = g_numSample < 1.0f ? 1 : STACK_DEPTH;
	float3 color = Raytrace(ray, threadIndex, numSamples);

	if(g_numSample < 1.0f)
	{
		for(uint x = 0; x < 2; x++)
		for(uint y = 0; y < 2; y++)
		g_OutBuff[dispatchThreadID.x + x + g_OutResolution.x * ( dispatchThreadID.y + y )] = color.xyzz;
		//g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = color.xyzz;a
	}
	else
	{
		float3 oldColor = g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y].xyz;
		//float3 newColor = (oldColor*(g_numSample-1) + color)/g_numSample;
		float3 newColor = lerp(oldColor, color, 1.0f/(g_numSample));
		g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = newColor.xyzz;
	}
}


#define not_empty_cell(X) ((X.x>0||X.y>0||X.z>0)&&(X.w>0))

static const int textureSize = 64, S = textureSize;
static const int maxSamples = textureSize;

bool getLight(Ray ray, out float dist, out float4 v, float maxL) // pos.xyz, t
{
	float3 startPos = ray.Origin;
	Box box, b;
	float3 info, p;
	float epsilon = 0.001f, ep = epsilon;
	box.Min = float3(0,0,0);
	box.Max = float3(textureSize,textureSize,textureSize);
	info = IntersectRayBoxEx(ray, box);
	if(info.x)
	{
		if(!IntersectPointBox(ray.Origin, box))
		{
			p = ray.Origin = ray.Origin + ray.Dir * ( info.y + ep );
			v = g_tex3d[((int)(p.x))*S*S + ((int)(p.y))*S +((int)(p.z))];
			if(not_empty_cell(v)){ dist = distance(p, ray.Origin); return true; }
		}

		for(int i = 0; i < 2; i++)
		{
			b.Min = int3(ray.Origin);
			b.Max = b.Min + 1;
			info = IntersectRayBoxEx(ray, b);
			p = ray.Origin = ray.Origin + ray.Dir * ( info.z + ep );
			if(IntersectPointBox(p, box))
			{
				v = g_tex3d[((int)(p.x))*S*S + ((int)(p.y))*S +((int)(p.z))];
				if(not_empty_cell(v)){ dist = distance(p, ray.Origin); return true; }		
			}
			else break;
		}
		return false;
	}
	else return false;
}

void addLight(float3 pos, inout float4 v) 
{
	Ray ray;
	ray.Origin = pos;
	float4 c, r = 0;
	float dist, maxL = 0.5;
	float co = 1;
	for(int i = 0; i < 20; i++)
	{		
		ray.Dir = normalize(random_vec());
		ray.invDir = 1 / ray.Dir;
		if(getLight(ray, dist, c, maxL) && dist < maxL && c.a > 0.5)
		{
			r +=  (1+c.a) * pow(c, 2) * ( 1.0f - dist / maxL );
			co++;
		}
	}
	v += (v.a > 0.5) ? (1+v.a * pow(v, 2)) : (0);
	v = lerp(v, pow(r / co, 2), 0.5);
}

void makeSphere(in Box box, inout Sphere sphere)
{
	sphere.Pos = (box.Max + box.Min) / 2;
	sphere.Radius = 1;
}

bool trace(Ray ray, out float3 finalPos, out float finalT, out float4 v, out float3 grad) // pos.xyz, t
{
	float3 startPos = ray.Origin;
	Box box, b;
	float3 info, p;
	float epsilon = 0.001f, ep = epsilon;
	box.Min = float3(0,0,0);
	box.Max = float3(textureSize,textureSize,textureSize);
	info = IntersectRayBoxEx(ray, box);
	Sphere sphere;
	if(info.x)
	{
		if(!IntersectPointBox(ray.Origin, box))
		{
			p = ray.Origin = ray.Origin + ray.Dir * ( info.y + ep );
			v = g_tex3d[((int)(p.x))*S*S + ((int)(p.y))*S +((int)(p.z))];
			if(not_empty_cell(v)){ grad = 1; finalPos = p; addLight(p, v); return true; }
		}

		for(int i = 0; i < maxSamples; i++)
		{
			b.Min = int3(ray.Origin);
			b.Max = b.Min + 1;
			info = IntersectRayBoxEx(ray, b);
			p = ray.Origin = ray.Origin + ray.Dir * ( info.z + ep );
			//if(IntersectPointBox(p, box))
			makeSphere(box, sphere);
			if(IntersectPointBox(p, box))
			{
				v = g_tex3d[((int)(p.x))*S*S + ((int)(p.y))*S +((int)(p.z))];
				if(not_empty_cell(v)){ grad = 1; finalPos = p; finalT = distance(startPos, finalPos); addLight(p, v);  return true; }		
			}
			else break;
		}
		return false;
	}
	else return false;
}

/*void bend(in out Ray ray, in out float bendStep, float3 org)
{ 
	float dist = distance(ray.Origin, org);
	if(dist<5.0f)
	{
		ray.Dir = normalize(ray.Dir +lerp(0.5f * normalize(org-ray.Origin)/pow(dist+4,2), 0, dist/5)); 
		ray.invDir = 1.0f / ray.Dir;
		bendStep = 0.05f;
	}
	else
	{
		bendStep = max(distance(ray.Origin, org)-4, 0.05f);
	}
}*/

float3 RayTrace(Ray ray, uint threadIndex, int max_bounces)
{
	float3	fP;
	float	fT;
	float4	v;
	float3	grad;
	if(trace(ray, fP, fT, v, grad))
		return lerp(v.rgb * grad, pow(v.rgb, 6), clamp(fT, 0.0f, 30.0f) / 30.0f );
	else return 0.05f;
}
/*
[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID,
		  uint3 groupID : SV_GroupID,
		  uint3 dispatchThreadID : SV_DispatchThreadID,
		  uint groupIndex : SV_GroupIndex)
{
	if (any(dispatchThreadID.xy >= g_OutResolution))
		return;
	seed = g_InitialSeed + g_RandSeed[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y];
	uint threadIndex = threadIDInGroup.x + THREADS_X * threadIDInGroup.y;
	float2 coords;
	Ray ray;
	float3 up = cross(g_CamDir.xyz, g_CamLeft.xyz);
	up = normalize(up);

	//float depth = g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y].w;
	float depth = 1;
	float w = 1.0f, w2 = 1.0f;
	float2 rnd_coords = (g_numSample >= 1.0f)?(w2*depth*randFloat2()-(w2*depth)/2):(float2(0,0));
	coords = (float2)((float2)dispatchThreadID.xy + rnd_coords) / (float)(g_OutResolution.xy);
	coords -= float2(0.5, 0.5);
	coords *= 2.0f;
	coords.x *= (float)g_OutResolution.y/(float)g_OutResolution.x;
	coords.y *= (float)g_OutResolution.y/(float)g_OutResolution.x;
	
	ray.Origin = g_CamPos.xyz;
	ray.Dir = g_CamDir.xyz + coords.x*g_CamLeft.xyz + coords.y*up;
	ray.Dir = normalize(ray.Dir);
	ray.invDir = 1 / ray.Dir;

	uint numSamples = g_numSample < 1.0f ? 1 : STACK_DEPTH;
	
	float3 color = RayTrace(ray, threadIndex, numSamples);//, depth);

	if(g_numSample < 1.0f)
	{
		g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = float4(color.xyz, depth);
	}
	else
	{
		g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y].xyz = lerp(g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y].xyz, color, 1.0f/(g_numSample)).xyz;
	}
}
*/
		/*
		float w = 1.0f, w2 = 1.0f;
		float2 rnd_coords = (g_numSample >= 1.0f)?(w2*randFloat2()-w2/2):(float2(0,0));
		coords = (float2)((float2)dispatchThreadID.xy + rnd_coords) / (float)(g_OutResolution.xy);
		coords -= float2(0.5, 0.5);
		coords.x *= g_OutResolution.y/g_OutResolution.x*2.0f;
		coords.y *= 2.0f;
	
		ray.Origin = g_CamPos.xyz;
		ray.Dir = g_CamDir.xyz + coords.x*g_CamLeft.xyz + coords.y*up;
		ray.Dir = normalize(ray.Dir);
		ray.invDir = 1 / ray.Dir;

		uint numSamples = g_numSample < 1.0f ? 1 : STACK_DEPTH;
		float3 color = RayTrace(ray, threadIndex, numSamples);

		if(g_numSample < 1.0f)
		{
			g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = color.xyzz;
		}
		else
		{
			g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y] = lerp(g_OutBuff[dispatchThreadID.x + g_OutResolution.x * dispatchThreadID.y].xyz, color, 1.0f/(g_numSample)).xyzz;
		}
		*/

/*

	if((color.r+color.g+color.b)<= 0.1f)
	color = float3(0.5,0.5,0.5);

	float3 boxIntersect = IntersectRayBoxEx(ray, box);
	if(boxIntersect.x)
	{
		color = float3(0,0,0);
		float steps = 16.0f;
		float stepsize = sqrt(3)/steps;

		float3 P, Pstep;
		P = ray.Origin + ray.Dir*boxIntersect.x;
		Pstep = ray.Dir*(stepsize);

		for(int i = 0; i < 512; i++)
		{
			if(IntersectPointBox(P, box))
			{
				float xx = (box.Max-box.Min)/(steps);
				float3 Pindex = (P-box.Min)/(box.Max-box.Min)*S;
				float4 s = g_tex3d[((int)Pindex.x)*S*S + ((int)Pindex.y)*S +(int)Pindex.z];
				color += s.rgb/256.0f;
				//break;
			}
			else color += 0.5f/512.0f;

			P += Pstep;
		}
		
		}*/