#pragma once

#include "Math.h"

class Camera
{
private:
	Frustum				m_frustum;
	Sphere				m_sphere;
	matrix				m_matProj;
	matrix				m_matView;
	matrix				m_matViewProj;
	bool				m_updateViewMat;

	f32					m_nearWidth;
	f32					m_nearHeight;
	f32					m_farWidth;
	f32					m_farHeight;
	f32					m_frustumCenterZ;

public:
	PerspectiveProjDesc	m_proj_desc;
	CameraViewDesc		m_view_desc;

	void* operator new(size_t size);
	void operator delete(void* ptr);

	Camera();
	Camera(const float3& pos, const float3& lookAtPos);

	void			rotateBy(const float3& angles);
	void			lookAt(const vector& pos);
	void			setPosition(const vector& pos);
	void			moveBy(const vector& dir);
	void			moveByRel(const float3& dir);
	void			update(u32 width, u32 height);

	matrix&			getMatView() { return m_matView; }
	matrix&			getMatProj() { return m_matProj; }
	matrix&			getMatViewProj() { return m_matViewProj; }
	float3			getPos() { return float3(XMVectorGetX(m_view_desc.pos), XMVectorGetY(m_view_desc.pos), XMVectorGetZ(m_view_desc.pos)); }

	void			calculateViewMat();
	void			calculateProjMat(u32 width, u32 height);
};