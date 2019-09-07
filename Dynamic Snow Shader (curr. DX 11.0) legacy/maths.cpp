#include "pch.h"
#include "maths.h"


maths::maths()
{
}


maths::~maths()
{
}

float maths::dot(XMVECTOR* v1, XMVECTOR* v2)
{
	return (v1->x * v2->x)+(v1->y*v2->y)+(v1->z*v2->z);
}

XMVECTOR maths::cross(XMVECTOR* v1, XMVECTOR* v2)
{
	XMVECTOR v3;
	v3.x = (v1->y * v2->z) - (v1->z * v2->y);
	v3.y = (v1->z * v2->x) - (v1->x * v2->z);
	v3.z = (v1->x * v2->y) - (v1->y * v2->x);
	v3.w = 0;
	return v3;
}

XMVECTOR maths::normal(XMVECTOR* v1, XMVECTOR* v2, XMVECTOR* v3)
{
	XMVECTOR p1, p2;
	p1.x = v2->x - v1->x;
	p1.y = v2->y - v1->y;
	p1.z = v2->z - v1->z;
	p2.z = v3->z - v1->z;
	p2.x = v3->x - v1->x;
	p2.y = v3->y - v1->y;
	p1.w = 0;
	p2.w = 0;
	XMVECTOR normal= cross(&p1, &p2);
	normal= XMVector3Normalize(normal);
	return normal;
}

maths::Plane maths::planeEquation(XMVECTOR* v1, XMVECTOR* v2, XMVECTOR* v3)
{
	Plane p;
	p.normal = normal(v1, v2, v3);
	p.d = -(dot(v1, &p.normal));
	return p;
}

XMVECTOR maths::planeIntersection(Plane * plane, XMVECTOR * p1, XMVECTOR * p2)
{
	XMVECTOR r;
	r.x = p2->x - p1->x;
	r.y = p2->y - p1->y;
	r.z = p2->z - p1->z;
	float t = (-(plane->d) - (dot(&plane->normal, p1) / dot(&plane->normal, &r)));
	XMVECTOR inter;
	inter.w = 0;
	if (t<0.0f || t>1.0f)
	{
		inter.x = 999999.0f;
		inter.y = 999999.0f;
		inter.z = 999999.0f;
		return inter;
	}
	else
	{
		inter.x = p1->x + (r.x*t);
		inter.y = p1->y + (r.y*t);
		inter.z = p1->z + (r.z*t);
		return inter;
	}
}

bool maths::in_triangle(XMVECTOR * triangle0, XMVECTOR * triangle1, XMVECTOR * triangle2, XMVECTOR * point)
{
	XMVECTOR AP, AB, BP, BC, CP, CA;
	AP = XMVectorSubtract(*triangle0, *point);
	AB = XMVectorSubtract(*triangle0, *triangle1);
	BP = XMVectorSubtract(*triangle1, *point);
	BC = XMVectorSubtract(*triangle1, *triangle2);
	CP = XMVectorSubtract(*triangle2, *point);
	CA = XMVectorSubtract(*triangle2, *triangle0);

	return dot(&AP, &AP) >= 0 && dot(&BP, &BC) >= 0 && dot(&CP, &CA)>= 0;
}

float maths::planeTest(Plane* plane, XMVECTOR* p1)
{
	return (plane->normal.x*p1->x)+(plane->normal.y*p1->y)+(plane->normal.z*p1->z)+plane->d;
}

int maths::sign(float number)
{
	return (number < 0.0 ? -1 : (number > 0.0f ? 1 : 0));
}


