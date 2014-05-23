#include "stdafx.h"
#include "Math.h"

XMVECTOR Box::getCenter()
{
	return (Min + Max) * 0.5f;
}

void Box::makeCube()
{
	XMVECTOR Center = getCenter();
	XMVECTOR HalfSize = Max - Center;
	float Size = max(XMVectorGetX(HalfSize), XMVectorGetY(HalfSize));
	Size = max(XMVectorGetZ(HalfSize), Size);
	Max = Min + XMVectorSet(Size, Size, Size, 0.0f) * 2.0f;
}

XMVECTOR Box::getVertex(u32 id)
{
	XMFLOAT3 result;
	result.x = (id & (1<<0)) ? XMVectorGetX(Max) : XMVectorGetX(Min);
	result.y = (id & (1<<1)) ? XMVectorGetY(Max) : XMVectorGetY(Min);
	result.z = (id & (1<<2)) ? XMVectorGetZ(Max) : XMVectorGetZ(Min);
	return XMLoadFloat3(&result);
}

void Box::createTriAABB(vector V1, vector V2, vector V3)
{
	Min = V1;
	Min = XMVectorMin(V2, Min);
	Min = XMVectorMin(V3, Min);

	Max = V1;
	Max = XMVectorMax(V2, Max);
	Max = XMVectorMax(V3, Max);
}

vector Box::getSurfaceArea()
{
	XMVECTOR size = Max - Min;
	XMVECTOR x = XMVectorSplatX(size);
	XMVECTOR y = XMVectorSplatY(size);
	XMVECTOR z = XMVectorSplatZ(size);
	return x*(y+z) + z*y;
}

vector Box::getVolume()
{
	XMVECTOR size = Max - Min;
	XMVECTOR x = XMVectorSplatX(size);
	XMVECTOR y = XMVectorSplatY(size);
	XMVECTOR z = XMVectorSplatZ(size);
	return x*y*z;
}



// ---------------------------------------------------------------
// Frustum
// ---------------------------------------------------------------

void Frustum::findAABB()
{
	AABB.Min = Points[0];
	AABB.Max = Points[0];

	for (int i = 1; i<8; i++)
	{
		AABB.Min = XMVectorMin(Points[i], AABB.Min);
		AABB.Max = XMVectorMax(Points[i], AABB.Max);
	}
}

XMVECTOR Frustum::supportVertex(XMVECTOR direction)
{
	XMVECTOR dir = XMVector3Normalize(direction);
	
	float DdP = XMVectorGetX(XMVector3Dot(Points[0], dir));
	int VertexId = 0;

	for (int i = 1; i<8; i++)
	{
		float TmpDpP = XMVectorGetX(XMVector3Dot(Points[i], dir));

		if (TmpDpP > DdP)
		{
			DdP = TmpDpP;
			VertexId = i;
		}
	}

	return Points[VertexId];
}


void Frustum::buildFromPerspectiveView(PerspectiveProjDesc& projDesc, CameraViewDesc &viewDesc)
{
	float h_near = projDesc.zNear * tanf(projDesc.fov*0.5f);
    float h_far = projDesc.zFar * tanf(projDesc.fov*0.5f);
    float w_near = h_near * projDesc.aspectRatio;
	float w_far = h_far * projDesc.aspectRatio;


	XMVECTOR zAxis = XMVector3Normalize(viewDesc.dir);
	XMVECTOR xAxis = XMVector3Normalize(XMVector3Cross(zAxis, viewDesc.up));
	XMVECTOR yAxis = XMVector3Normalize(XMVector3Cross(xAxis, zAxis));
	XMVECTOR n_c = viewDesc.pos + zAxis * projDesc.zNear;
    XMVECTOR f_c = viewDesc.pos + zAxis * projDesc.zFar;

    Points[3] = n_c;                 //near top right
	Points[3] += yAxis * h_near;
    Points[3] += xAxis * w_near;

    Points[2] = n_c;                 //near top left
    Points[2] += yAxis * h_near;
    Points[2] -= xAxis * w_near;

	Points[1] = n_c;                 //near bottom right
	Points[1] -= yAxis * h_near;
	Points[1] += xAxis * w_near;

	Points[0] = n_c;                 //near bottom left
	Points[0] -= yAxis * h_near;
	Points[0] -= xAxis * w_near;


	Points[7] = f_c;                 //far top right
	Points[7] += yAxis * h_far;
	Points[7] += xAxis * w_far;

	Points[6] = f_c;                 //far top left
	Points[6] += yAxis * h_far;
	Points[6] -= xAxis * w_far;

	Points[5] = f_c;                 //far bottom right
	Points[5] -= yAxis * h_far;
	Points[5] += xAxis * w_far;

	Points[4] = f_c;                 //far bottom left
	Points[4] -= yAxis * h_far;
	Points[4] -= xAxis * w_far;

	
	Planes[0] = XMPlaneFromPoints(Points[0], Points[1], Points[3]); //front
	Planes[1] = XMPlaneFromPoints(Points[4], Points[2], Points[6]); //left
	Planes[2] = XMPlaneFromPoints(Points[7], Points[3], Points[5]); //right
	Planes[3] = XMPlaneFromPoints(Points[6], Points[3], Points[7]); //top
	Planes[4] = XMPlaneFromPoints(Points[4], Points[5], Points[1]); //bottom
	Planes[5] = XMPlaneFromPoints(Points[6], Points[7], Points[4]); //far

	findAABB();
}





int Math::insideBoxBox(Box &innerBox, Box &outerBox)
{
	return (XMComparisonAllTrue(XMVector3GreaterOrEqualR(innerBox.Min, outerBox.Min)) &&
			XMComparisonAllTrue(XMVector3GreaterOrEqualR(outerBox.Max, innerBox.Max)));
}

int Math::intersectBoxBox(Box &box1, Box &box2)
{
	if (XMComparisonAnyTrue(XMVector3GreaterR(box1.Min, box2.Max)))
		return 0;

	if (XMComparisonAnyTrue(XMVector3GreaterR(box2.Min, box1.Max)))
		return 0;

	return 1;
}

int Math::intersectBoxPoint(Box &box, XMVECTOR point)
{
	if (XMComparisonAnyTrue(XMVector3GreaterR(point, box.Max)))
		return 0;

	if (XMComparisonAnyTrue(XMVector3GreaterR(box.Min, point)))
		return 0;

	return 1;
}


inline int Math_Plane_Frustum_Side(Frustum &frustum, XMVECTOR &plane)
{
	int pos_counter = 0;
	float dist;

	for (int i = 0; i<8; i++)
	{
		dist = XMVectorGetX(XMPlaneDotCoord(plane, frustum.Points[i]));
		if (dist > 0.0f) pos_counter++;
	}

	if (pos_counter == 8) return 1;
	return 0;
}


int Math::intersectFrustumBox(Frustum &frustum, Box &box)
{
	for (int i = 0; i<6; i++)
	{
		int counter = 0;
		for (int j = 0; j<8; j++)
		{
			XMVECTOR vertex = box.getVertex(j);
			if (XMVectorGetX(XMPlaneDotCoord(frustum.Planes[i], vertex)) > 0.0f) 
				counter++;
		}
			
		if (counter == 8)
			return 0;
	}


	XMVECTOR Plane;

	// X
	Plane = Math::vector4(1.0f, 0.0f, 0.0f, -XMVectorGetX(box.Max));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;

	Plane = Math::vector4(-1.0f, 0.0f, 0.0f, XMVectorGetX(box.Min));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;

	// Y
	Plane = Math::vector4(0.0f, 1.0f, 0.0f, -XMVectorGetY(box.Max));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;

	Plane = Math::vector4(0.0f, -1.0f, 0.0f, XMVectorGetY(box.Min));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;

	// Z
	Plane = Math::vector4(0.0f, 0.0f, 1.0f, -XMVectorGetZ(box.Max));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;

	Plane = Math::vector4(0.0f, 0.0f, -1.0f, XMVectorGetZ(box.Min));
	if (Math_Plane_Frustum_Side(frustum, Plane) == 1) return 0;


	return 1;
}



int Math::insideBoxFrustum(Box &box, Frustum &frustum)
{
	for (int i = 0; i<6; i++)
	{
		for (int j = 0; j<8; j++)
		{
			XMVECTOR vertex = box.getVertex(j);
			if (XMVectorGetX(XMVector3Dot(frustum.Points[i], vertex)) + XMVectorGetW(frustum.Points[i]) > 0.0f)
				return 0;
		}	
	}

	return 1;
}


int Math::intersectFrustumFrustum(Frustum &frustum1, Frustum &frustum2)
{
	if (intersectBoxBox(frustum1.AABB, frustum2.AABB) == 0)
		return 0;

	int i, j, count;
	for (i = 0; i<6; i++)
	{
		count = 0;
        for (j = 0; j<8; j++)
		{
			if (XMVectorGetX(XMVector3Dot(frustum1.Planes[i], frustum2.Points[j])) > 0.0f)
				count++;
		}

        if (count==8) return 0;
	}

    return 1;
}

int Math::intersectFrustumPoint(Frustum &frustum, XMVECTOR point)
{
	int i, j, count;
	for (i = 0; i<6; i++)
	{
		count = 0;
        for (j = 0; j<8; j++)
		{
			if (XMVectorGetX(XMPlaneDotCoord(frustum.Planes[i], point)) > 0.0f)
				count++;
		}

        if (count==8) return 0;
	}

	return 1;
}



int Math::intersectFrustumSphereFast(Frustum &frustum, Sphere &sphere)
{
	for (int i = 0; i<6; i++)
	{
		if ( XMVectorGetX(XMPlaneDotCoord(frustum.Planes[i], sphere.Pos)) > sphere.Radius)
			return 0;
	}
	return 1;
}



/*


float Math_Ray_AABB_Intersection(Ray *Ray, Box *Box)
{
	XMVECTOR P;
	float tmpt;

    if (ray.Origin[0]>box.Max[0])
    {
      if (ray.Dir[0]<0.0f)
      {

        if (ray.Dir[0] != 0.0f)
            tmpt = (-ray.Origin[0] +box.Max[0]) / ray.Dir[0];
        else
          return -1.0f;


        if (tmpt>0.0f)
        {
            P.data[1] = ray.Origin[1] + tmpt*ray.Dir[1];
            P.data[2] = ray.Origin[2] + tmpt*ray.Dir[2];

            if (P.data[1]<=box.Max[1])
              if (P.data[1]>=box.Min[1])
				if(P.data[2]<=box.Max[2])
				if  (P.data[2]>=box.Min[2])
					return tmpt;

        }
        else
			return -1.0f;
      }
      else
		  return -1.0f;
    }

    //X-
    if (ray.Origin[0]<box.Min[0])
    {
      if (ray.Dir[0]>0.0f)
      {
        if (ray.Dir[0] != 0.0f)
            tmpt = (-ray.Origin[0] + box.Min[0]) / ray.Dir[0];
        else
			return -1.0f;

        if (tmpt>0.0f)
        {
            P.data[1] = ray.Origin[1] + tmpt*ray.Dir[1];
            P.data[2] = ray.Origin[2] + tmpt*ray.Dir[2];

            if (P.data[1]<=box.Max[1])
				if  (P.data[1]>=box.Min[1])
					if (P.data[2]<=box.Max[2])
						if  (P.data[2]>=box.Min[2])
							return tmpt;

        }
        else
			return -1.0f;
      }
      else
		  return -1.0f;
    }


    //Y+
    if (ray.Origin[1]>box.Max[1])
    {
      if (ray.Dir[1]<0.0f)
      {
        if (ray.Dir[1] != 0.0f)
            tmpt = (-ray.Origin[1] + box.Max[1]) / ray.Dir[1];
        else
			return -1.0f;

        if (tmpt>0.0f)
        {
            P.data[0] = ray.Origin[0] + tmpt*ray.Dir[0];
            P.data[2] = ray.Origin[2] + tmpt*ray.Dir[2];

            if (P.data[0]<=box.Max[0])
              if (P.data[0]>=box.Min[0])
              if (P.data[2]<=box.Max[2])
              if (P.data[2]>=box.Min[2])
				  return tmpt;

        }
        else
          return -1.0f;
      }
      else
        return -1.0f;
    }

    //Y-
    if (ray.Origin[1]<box.Min[1])
    {
      if (ray.Dir[1]>0.0f)
      {
        if (ray.Dir[1] != 0.0f)
            tmpt = (-ray.Origin[1] + box.Min[1]) / ray.Dir[1];
        else
			return -1.0f;

        if (tmpt>0.0f)
        {
            P.data[0] = ray.Origin[0] + tmpt*ray.Dir[0];
            P.data[2] = ray.Origin[2] + tmpt*ray.Dir[2];

            if (P.data[0]<=box.Max[0])
              if (P.data[0]>=box.Min[0])
              if (P.data[2]<=box.Max[2])
              if (P.data[2]>=box.Min[2])
				  return tmpt;

        }
        else
			return -1.0f;
      }
      else
		  return -1.0f;
    }


    //Z+
    if (ray.Origin[2]>box.Max[2])
    {
      if (ray.Dir[2]<0)
      {
        if (ray.Dir[2] != 0.0f)
            tmpt = (-ray.Origin[2] + box.Max[2]) / ray.Dir[2];
        else
			return -1.0f;


        if (tmpt>0.0f)
        {
            P.data[0] = ray.Origin[0] + tmpt*ray.Dir[0];
            P.data[1] = ray.Origin[1] + tmpt*ray.Dir[1];

            if (P.data[0]<=box.Max[0])
              if (P.data[0]>=box.Min[0])
              if (P.data[1]<=box.Max[1])
              if (P.data[1]>=box.Min[1])
				  return tmpt;

        }
        else
          return -1.0f;
      }
    }

    //Z-
    if (ray.Origin[2]<box.Min[2])
    {
      if (ray.Dir[2]>0.0f)
      {
        if (ray.Dir[2] != 0.0f)
            tmpt = (-ray.Origin[2] + box.Min[2]) / ray.Dir[2];
        else
			return -1.0f;


        if (tmpt>0.0f)
        {
            P.data[0] = ray.Origin[0] + tmpt*ray.Dir[0];
            P.data[1] = ray.Origin[1] + tmpt*ray.Dir[1];

            if (P.data[0]<=box.Max[0])
				if (P.data[0]>=box.Min[0])
					if (P.data[1]<=box.Max[1])
						if (P.data[1]>=box.Min[1])
							return tmpt;

        }
      }
    }

	return -1.0f;
}
*/


bool Math::intersectRayBox(Box &box, Ray &ray) 
{
	XMVECTOR DirRcp = XMVectorReciprocal(ray.Dir);
	XMVECTOR l1 = XMVectorMultiply(box.Min - ray.Origin, DirRcp);
	XMVECTOR l2 = XMVectorMultiply(box.Max - ray.Origin, DirRcp);
	
	float lmin = min(XMVectorGetX(l1), XMVectorGetX(l2));
	float lmax = max(XMVectorGetX(l1), XMVectorGetX(l2));

	lmin = max(lmin, min(XMVectorGetY(l1), XMVectorGetY(l2)));
	lmax = min(lmin, max(XMVectorGetY(l1), XMVectorGetY(l2)));

	lmin = max(lmin, min(XMVectorGetZ(l1), XMVectorGetZ(l2)));
	lmax = min(lmin, max(XMVectorGetZ(l1), XMVectorGetZ(l2)));

	//return ((lmax > 0.f) & (lmax >= lmin));
	//return ((lmax > 0.f) & (lmax > lmin));
	return ((lmax >= 0.f) & (lmax >= lmin));
}


float Math::intersectRaySphere(Ray &ray, Sphere &sphere)
{
	float B = XMVectorGetX(XMVector3Dot(ray.Origin-sphere.Pos, ray.Dir));
	XMVECTOR dir = ray.Origin - sphere.Pos;
	float D = B*B - XMVectorGetX(XMVector3Dot(dir, dir)) + sphere.Radius*sphere.Radius;
	return (D > 0) ? -B - sqrtf(D) : -1.0f;
}


float Math::intersectRayTriangle(Ray &ray, Triangle &tri)
{
	XMVECTOR pvec = XMVector3Cross(ray.Dir, tri.V2 - tri.V0);

	float det = XMVectorGetX(XMVector3Dot(tri.V1 - tri.V0, pvec));

	if ((det<MATH_EPSILON) && (det>-MATH_EPSILON))
		return -1.0f;

	float invDet = 1.0f/det;

	XMVECTOR tvec = ray.Origin - tri.V0;
	float u = XMVectorGetX(XMVector3Dot(tvec, pvec)) * invDet;

    if (u>1.0f)
      return -1.0f;
    if (u<0.0f)
      return -1.0f;


	XMVECTOR qvec = XMVector3Cross(tvec, tri.V1 - tri.V0);
	float v = XMVectorGetX(XMVector3Dot(ray.Dir, qvec)) * invDet;

	if (v<0.0f)
		return -1.0f;
	if (u + v > 1.0f)
		return -1.0f;

	return XMVectorGetX(XMVector3Dot(tri.V2-tri.V0, qvec)) * invDet;
}



bool Math::intersectRayTriangleEx(Ray &ray, Triangle &tri, RayIntersectionInfo* pInfo)
{
	XMVECTOR pvec = XMVector3Cross(ray.Dir, tri.V2 - tri.V0);
	float det = XMVectorGetX(XMVector3Dot(tri.V1 - tri.V0, pvec));

	if ((det<MATH_EPSILON) && (det>-MATH_EPSILON))
		return 0;

	float invDet = 1.0f/det;

	XMVECTOR tvec = ray.Origin - tri.V0;
	float u = XMVectorGetX(XMVector3Dot(tvec, pvec)) * invDet;

    if (u>1.0f || u<0.0f)
      return 0;


	XMVECTOR qvec = XMVector3Cross(tvec, tri.V1 - tri.V0);
	float v = XMVectorGetX(XMVector3Dot(ray.Dir, qvec)) * invDet;

	if (v<0.0f || u + v > 1.0f)
		return 0;

	if (pInfo)
	{
		pInfo->Dist = XMVectorGetX(XMVector3Dot(tri.V2-tri.V0, qvec)) * invDet;
		pInfo->u = u;
		pInfo->v = v;
	}
	return 1;
}