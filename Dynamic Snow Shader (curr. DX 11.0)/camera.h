#pragma once
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include "pch.h"
#include "scene_node.h"


class camera
{
private:
	//m_camera_rotation in degrees
	float m_x, m_y, m_z, m_dx, m_dy, m_dz, m_camera_rotationY, m_camera_rotationX;
	XMVECTOR m_position, m_lookat, m_up, m_right;
	scene_node* root;
	bool active;

public:
	camera();
	camera(float x, float y, float z, float camera_rotationY, float camera_rotationX, scene_node* root_node, bool state);
	void Rotate(float camera_rotationY, float camera_rotationX);
	void Forward(float distance);
	void Up(float up);
	void Strafe(float distance);
	void Jump(float distance);
	XMVECTOR GetOrthogonal();
	XMMATRIX GetViewMatrix();
	XMMATRIX GetProjMatrix();
	XMVECTOR GetCameraPosition();
	XMVECTOR GetLookAt();
	float GetX();
	float GetY();
	float GetZ();
	void SetActive(bool state);
	bool GetActive();

	//update internal values
	void UpdatePosition();
	void UpdateLookaAt();
	~camera();
};

