#include "camera.h"



camera::camera()
{
}

camera::camera(float x, float y, float z, float camera_rotationY, float camera_rotationX, scene_node* root_node, bool state)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_camera_rotationY = camera_rotationY;
	m_camera_rotationX = camera_rotationX;
	m_dx = sin(m_camera_rotationY*(XM_PI / 180));
	m_dy = sin(m_camera_rotationX*(XM_PI / 180));
	m_dz = cos(m_camera_rotationY*(XM_PI / 180));
	active = state;
	root = root_node;
	UpdateLookaAt();
	UpdatePosition();
}

//positive values rotate to the right for rotationY and up for rotationX
void camera::Rotate(float camera_rotationY, float camera_rotationX)
{
	m_camera_rotationX += camera_rotationX;
	m_camera_rotationY -= camera_rotationY;
	
	if (-95 > m_camera_rotationX) m_camera_rotationX += 1;
	if (m_camera_rotationX > 90) m_camera_rotationX += -1;
	m_dx = sin(m_camera_rotationY*(XM_PI / 180));
	m_dy = sin(m_camera_rotationX*(XM_PI / 180));
	m_dz = cos(m_camera_rotationY*(XM_PI / 180));
	m_lookat = XMVectorSet(m_x + m_dx, m_y + m_dy, m_z + m_dz, 0.0f);
	m_lookat -= m_position;
	m_right = XMVector3Cross(m_lookat, m_up);
	
}

//postitive values move the camera forward
void camera::Forward(float distance)
{
	XMVECTOR normalized;
	normalized = XMVector3Normalize(m_lookat);
	normalized.x = normalized.x * distance;
	normalized.z = normalized.z * distance;
	if(!root->checkCollisionRay(&m_position, &normalized, true))
	{
		m_x += m_dx*distance;
		m_z += m_dz*distance;
		UpdatePosition();
		UpdateLookaAt();
	}
}


void camera::Up(float up)
{
	m_y = up;
}

void camera::Strafe(float distance)
{
	XMVECTOR orth = -XMVector3Normalize(GetOrthogonal());
	orth.x = distance*orth.x;
	orth.z = distance*orth.z;
	if(!root->checkCollisionRay(&m_position, &orth, true))
	{
		m_x += orth.x;
		m_z += orth.z;
		UpdatePosition();
		UpdateLookaAt();
	}
	
}

void camera::Jump(float distance)
{
	m_y += distance;
	UpdatePosition();
	UpdateLookaAt();
}

XMVECTOR camera::GetOrthogonal()
{
	UpdateLookaAt();
	m_right = XMVector3Cross(m_lookat, m_up);
	return m_right;
}

XMMATRIX camera::GetViewMatrix()
{
	XMMATRIX view;
	m_position = XMVectorSet(m_x, m_y, m_z, 0.0f);
	m_lookat = XMVectorSet(m_x + m_dx, m_y + m_dy, m_z+ m_dz,0.0f);
	m_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_right = XMVector3Cross(m_lookat, m_up);
	view = XMMatrixLookAtLH(m_position, m_lookat, m_up);
	return view;
}

XMMATRIX camera::GetProjMatrix()
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(65.0f), 640.0f / 480.0f, 1.0f, 1000.0f);
	return proj;
}

XMVECTOR camera::GetCameraPosition()
{
	return m_position;
}

XMVECTOR camera::GetLookAt()
{
	return m_lookat;
}

float camera::GetX()
{
	return m_x;
}

float camera::GetY()
{
	return m_y;
}

float camera::GetZ()
{
	return m_z;
}

void camera::SetActive(bool state)
{
	active= state;
}

bool camera::GetActive()
{
	return active;
}

void camera::UpdatePosition()
{
	m_position = XMVectorSet(m_x, m_y, m_z, 0.0f);
}

void camera::UpdateLookaAt()
{
	m_lookat = XMVectorSet(m_x + m_dx, m_y + m_dy, m_z + m_dz, 0.0f);
	m_lookat -= m_position;
}


camera::~camera()
{
	if (root) root = nullptr;
}
