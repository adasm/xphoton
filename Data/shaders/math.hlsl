
static uint2 seed;

static const float PI = 3.14159265f;
static const float PI2 = 6.28318531f;

struct Box
{
	float3 Min;
	float3 Max;
};

struct Sphere
{
	float3 Pos;
	float  Radius;
};

struct Tri
{
	float3 V0;
	float3 V1;
	float3 V2;
};

struct Ray
{
	float3 Origin;
	float3 Dir;
	float3 invDir;
};

struct IntersectInfo
{
	float t;
	int triIndex;
	float2 uv;
};


//returns 0/1
int IntersectRayBox(Ray ray, Box box) 
{
	float3 l1 = (box.Min - ray.Origin) * ray.invDir;
	float3 l2 = (box.Max - ray.Origin) * ray.invDir;
	
	float2 dist;

	float3 lmin = min(l1, l2);
	dist.x = max(lmin.y, lmin.z);
	dist.x = max(lmin.x, dist.x);

	float3 lmax = max(l1, l2);
	dist.y = min(lmax.y, lmax.z);
	dist.y = min(lmax.x, dist.y);

	return all(dist.yy >= float2(0, dist.x));
}


//returns float3([collision occurs], dist to the nearest side, dist to the furthest side)
float3 IntersectRayBoxEx(Ray ray, Box box) 
{
	float3 l1 = (box.Min - ray.Origin) * ray.invDir;
	float3 l2 = (box.Max - ray.Origin) * ray.invDir;
	
	float3 lmin = min(l1, l2);
	float3 lmax = max(l1, l2);

	float2 lx = float2(lmin.x, lmax.x);
	float2 ly = float2(lmin.y, lmax.y);
	float2 lz = float2(lmin.z, lmax.z);
	lmin.x = max(lx, max(ly, lz)).x;
	lmax.x = min(lx, min(ly, lz)).y;

	return float3(((lmax.x >= 0) && (lmax.x >= lmin.x)), lmin.x, lmax.x);
}

float IntersectRaySphere(Ray ray, Sphere sphere)
{
	float B = dot(ray.Origin - sphere.Pos, ray.Dir).x;
	float3 dir = ray.Origin - sphere.Pos;
	float D = B*B - dot(dir, dir) + sphere.Radius*sphere.Radius;
	return (D > 0) ? -B - sqrt(D) : -1.0f;
}

bool IntersectPointSphere(float3 point_, Sphere sphere)
{
	if( length(point_ - sphere.Pos) <= sphere.Radius )
	return true;
	else return false;
}

bool IntersectPointBox(float3 point_, Box box) 
{
	return all((point_ > box.Min) && (point_<box.Max));
}

float IntersectRayTri(Ray ray, Tri tri, out float2 uv)
{
	float3 edge0 = tri.V1 - tri.V0;
	float3 edge1 = tri.V2 - tri.V0;

	float3 pvec = cross(ray.Dir, edge1);

	float det = dot(edge0, pvec);
	float invDet = 1.0f / det;

	float3 tvec = ray.Origin - tri.V0;
	float u = dot(tvec, pvec) * invDet;

	float3 qvec = cross(tvec, edge0);
	float v = dot(ray.Dir, qvec) * invDet;

	uv = float2(u, v);
	return all(float3(u, v, 1) >= float3(0, 0, u+v)) * dot(edge1, qvec) * invDet;
}

// PSEUDO RANDOM NUMBER GENERATOR

uint randUint()
{
	seed.x = seed.x * 196314165 + 907633515;
	return seed.x;
}

float randFloat()
{
	return (float)(randUint()%100000) * 0.00001f;
}

float2 randFloat2()
{
	return float2(randFloat(), randFloat());
}


//random unit vector on sphere
float3 random_vec()
{
	//float2 u = randFloat2();
	float2 u;
	u.x = 2.0f * randFloat() - 1.0f;
	u.y = randFloat();

	float r = sqrt(1.0f - u.x*u.x);

	float theta = PI2 * u.y;
	return float3(sin(theta)*r, cos(theta)*r, u.x);
}

float3 randomVecOnHemisphere()
{
	float2 u;
	u.x = randFloat();
	u.y = randFloat();
	float r = sqrt(u.x);

	float theta = 6.28318531f * u.y;
	float2 theta_sincos;
	sincos(theta, theta_sincos.x, theta_sincos.y);

	return float3(theta_sincos.xy, 1) * float3(r.xx, sqrt(1.0f-u.x));
}

