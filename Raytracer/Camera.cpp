#include "stdafx.h"
#include "Camera.h"

const float default_fov = XM_2PI / 1.8f;


void* Camera::operator new(size_t size)
{
	return _aligned_malloc(size, 16);
}

void Camera::operator delete(void* ptr)
{
	_aligned_free(ptr);
}


Camera::Camera()
{
	m_view_desc.up  = Math::vector3(0, 1, 0);
	m_view_desc.pos = Math::vector3(0, 0, 0);
	m_view_desc.dir = Math::vector3(0, 0, 1);
	m_proj_desc.fov = default_fov;
	m_updateViewMat = true;
	update(512, 512);
}

Camera::Camera(const float3& pos, const float3& lookAtPos)
{
	m_view_desc.up  = Math::vector3(0, 1, 0);
	m_view_desc.pos = Math::vector3(pos.x, pos.y, pos.z);
	m_view_desc.dir = Math::vector3(lookAtPos.x - pos.x, lookAtPos.y - pos.y, lookAtPos.z - pos.z);
	m_view_desc.dir = XMVector3Normalize(m_view_desc.dir);
	m_proj_desc.fov = default_fov;
	m_updateViewMat = true;
	update(512, 512);
}

void Camera::rotateBy(const float3& angles)
{
	matrix mR;
	mR = XMMatrixRotationAxis(m_view_desc.up, angles.y);
	m_view_desc.dir = ::XMVector3TransformCoord(m_view_desc.dir, mR);
	m_view_desc.left = ::XMVector3TransformCoord(m_view_desc.left, mR);
	mR = XMMatrixRotationAxis(m_view_desc.left, angles.x);
	m_view_desc.dir = XMVector3TransformCoord(m_view_desc.dir, mR);
	m_view_desc.dir = XMVector3Normalize(m_view_desc.dir);
	m_updateViewMat = true;
}

void Camera::lookAt(const vector& pos)
{
	m_view_desc.dir = XMVector3Normalize(pos - m_view_desc.pos);
	m_updateViewMat = true;
}

void Camera::setPosition(const vector& pos)
{
	m_view_desc.pos = pos;
	m_updateViewMat = true;
}

void Camera::update(u32 width, u32 height)
{
	calculateProjMat(width, height);
	calculateViewMat();
}

void Camera::calculateViewMat()
{
	if(m_updateViewMat)
	{
		m_view_desc.left = XMVector3Cross(m_view_desc.up, m_view_desc.dir);
		m_view_desc.left = XMVector3Normalize(m_view_desc.left);

		XMVectorSetW(m_view_desc.dir, 1);
		XMVectorSetW(m_view_desc.pos, 1);
		XMVectorSetW(m_view_desc.up, 0);
		XMVectorSetW(m_view_desc.left, 0);

		vector LookAt = XMVectorAdd(m_view_desc.pos, m_view_desc.dir);
		m_matView = XMMatrixLookAtLH(m_view_desc.pos, LookAt, m_view_desc.up);
		m_matViewProj = XMMatrixMultiply(m_matView, m_matProj);
		m_frustum.buildFromPerspectiveView(m_proj_desc, m_view_desc);
		m_updateViewMat = false;
	}
}

void Camera::calculateProjMat(u32 width, u32 height)
{
	m_proj_desc.aspectRatio = (f32)width  / (f32)height;
	m_proj_desc.zNear = 0.1f;
	m_proj_desc.zFar = 1000.0f;
	m_matProj = XMMatrixPerspectiveFovLH(m_proj_desc.fov, m_proj_desc.aspectRatio, m_proj_desc.zNear, m_proj_desc.zFar);
	//calculate sizes of frustum
	f32 t = (f32)tanf(m_proj_desc.fov * 0.5f);
	m_farHeight =  m_proj_desc.zFar * t;
	m_farWidth = m_proj_desc.aspectRatio * m_farHeight;
	m_nearHeight =  m_proj_desc.zNear * t;
	m_nearWidth = m_proj_desc.aspectRatio * m_nearHeight;
	//calculate frustum sphere radius
	m_frustumCenterZ = (m_proj_desc.zFar +  m_proj_desc.zNear) * 0.5f;
	m_sphere.Pos = m_view_desc.pos;
	m_sphere.Radius = XMVectorGetX(XMVector3Length(Math::vector3(m_farWidth, m_farHeight, (m_proj_desc.zFar - m_proj_desc.zNear) * 0.5f)));
}

void Camera::moveBy(const vector& dir)
{
	m_view_desc.pos = XMVectorAdd(m_view_desc.pos, dir);
	m_updateViewMat = true;
}

void Camera::moveByRel(const float3& dir)
{
	vector vx = XMVectorSet(dir.x, dir.x, dir.x, dir.x);
	vector vy = XMVectorSet(dir.y, dir.y, dir.y, dir.y);
	vector vz = XMVectorSet(dir.z, dir.z, dir.z, dir.z);
	
	vx = XMVectorMultiply(vx, m_view_desc.left);
	m_view_desc.pos = XMVectorAdd(m_view_desc.pos, vx);

	vy = XMVectorMultiply(vy, m_view_desc.up);
	m_view_desc.pos = XMVectorAdd(m_view_desc.pos, vy);

	vz = XMVectorMultiply(vz, m_view_desc.dir);
	m_view_desc.pos = XMVectorAdd(m_view_desc.pos, vz);

	m_updateViewMat = true;
}