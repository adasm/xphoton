#pragma once

struct PerspectiveProjDesc
{
	float fov;
	float aspectRatio;
	float zNear;
	float zFar;
};

struct CameraViewDesc
{
	vector pos;
	vector left;
	vector dir;
	vector up;
};

struct Box
{
	vector Min;
	vector Max;
	
	Box() {};
	Box(vector min, vector max) : Min(min), Max(max) {};		

	vector	getCenter();
	vector	getVertex(u32 id);
	void	makeCube();
	void	createTriAABB(XMVECTOR V1, XMVECTOR V2, XMVECTOR V3);

	vector getSurfaceArea();
	vector getVolume();
};

struct Frustum
{
	vector Points[8];
	vector Planes[6];
	Box AABB;

	void	buildFromPerspectiveView(PerspectiveProjDesc& projDesc, CameraViewDesc &viewDesc);
	void	findAABB();
	vector	supportVertex(vector direction);
};


struct Ray
{
	vector Dir;
	vector Origin;
};

struct Triangle
{
	vector V0;
	vector V1;
	vector V2;
};

struct Sphere
{
	vector Pos;
	float Radius;
};

struct RayIntersectionInfo
{
	float Dist;
	float u;
	float v;
};

#define MATH_EPSILON (0.000001f)

struct Math
{
	static inline vector vector3(float x, float y, float z) { return XMLoadFloat3(&XMFLOAT3(x, y, z)); }
	static inline vector vector4(float x, float y, float z, float w) { return XMLoadFloat4(&XMFLOAT4(x, y, z, w)); }

	static int		insideBoxBox(Box &innerBox, Box &outerBox);
	static int		intersectBoxBox(Box &box1, Box &box2);
	static int		intersectBoxPoint(Box &box, XMVECTOR point);

	static int		intersectFrustumPoint(Frustum &frustum, XMVECTOR point);
	static int		intersectFrustumBox(Frustum &frustum, Box &box);
	static int		insideBoxFrustum(Box &box, Frustum &frustum);
	static int		intersectFrustumFrustum(Frustum &frustum1, Frustum &frustum2);
	static int		intersectFrustumSphereFast(Frustum &frustum, Sphere &sphere);
	//this functions returns distance form origin of the ray to the point of collsion
	//if collsion does not occur, the function returns negative value
	static float	intersectRayTriangle(Ray &ray, Triangle &tri);
	static float	intersectRaySphere(Ray &ray, Sphere &sphere);
	static bool		intersectRayTriangleEx(Ray &ray, Triangle &tri, RayIntersectionInfo* pInfo);
	static bool		intersectRayBox(Box &box, Ray &ray);
};