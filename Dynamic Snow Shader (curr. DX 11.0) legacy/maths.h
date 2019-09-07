#pragma once
class maths
{

public:
	struct Plane
	{
		XMVECTOR normal;
		float d;
	};
	maths();
	~maths();
	float dot(XMVECTOR* v1, XMVECTOR* v2);
	XMVECTOR cross(XMVECTOR* v1, XMVECTOR* v2);
	XMVECTOR normal(XMVECTOR* v1, XMVECTOR* v2, XMVECTOR* v3);
	Plane planeEquation(XMVECTOR* v1, XMVECTOR* v2, XMVECTOR* v3);
	float planeTest(Plane* plane, XMVECTOR* p1);
	XMVECTOR planeIntersection(Plane* plane, XMVECTOR* p1, XMVECTOR* p2);
	bool in_triangle(XMVECTOR* triangle0, XMVECTOR* triangle1, XMVECTOR* triangle2, XMVECTOR* point);
	int sign(float number);

};

