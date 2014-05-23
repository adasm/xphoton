#pragma once

#include "model_obj.h"
#include "Camera.h"
#include "BIH.h"


#define MAT_DIFFUSE (1<<0)
#define MAT_REFLECTION (1<<1)

struct Material
{
	u32 flags;
	float3 diffuse_color;
	float3 reflection_color;
	float  glossy_factor; //0...1
	float3 emission_color;
	
};

struct Vertex { float3 position; float3 normal; float3 tangent; };
struct Face { u32 i0, i1, i2; u32 matIndex; };

struct BIHNode
{
	float3 box_min;
	float3 box_max;
	u32 index[2];
	u32 count;
	i32 successJump;
	i32 failureJump;
};


struct Light
{
	float3 pos;
	float3 color;
	float radius;
};

class Scene
{
public:
	rtBIHLeaf* m_pBIHLeafs;
	rtBIH* m_pBIH;

	rtBIHLeaf* m_pLightBIHLeafs;
	rtBIH* m_pLightBIH;

	std::vector<Vertex> vertex_buffer;
	std::vector<Face> face_buffer;
	std::vector<BIHNode> bih_buffer;
	std::vector<Material> material_buffer;

	std::vector<Light> light_buffer;
	std::vector<BIHNode> lightBih_buffer;

	Camera cam;

	Scene();
	~Scene();
	void load(const char* pFileName);
	void buildBihBuffer(rtBIHNode* pNode);

	void buildLightsBih();
	void buildLightBihBuffer(rtBIHNode* pNode);
};