#include "quat_utils.h"

double DegToRad(int degrees) {
	return degrees * M_PI / 180;
}
double RadToDeg(double rad) {
	return rad * 180 / M_PI;
}

vr::HmdQuaternion_t GetRotation(vr::HmdMatrix34_t matrix) {
	vr::HmdQuaternion_t q;

	q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;

	q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
	q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
	q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);

	return q;
}

vr::HmdMatrix33_t Get33Matrix(vr::HmdMatrix34_t matrix) {
	vr::HmdMatrix33_t result = { {
		{matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]},
		{matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]},
		{matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]}
		} };

	return result;
}
vr::HmdVector3_t MultiplyMatrix(vr::HmdMatrix33_t matrix, vr::HmdVector3_t vector) {
	vr::HmdVector3_t result;

	result.v[0] = matrix.m[0][0] * vector.v[0] + matrix.m[0][1] * vector.v[1] + matrix.m[0][2] * vector.v[2];
	result.v[1] = matrix.m[1][0] * vector.v[0] + matrix.m[1][1] * vector.v[1] + matrix.m[1][2] * vector.v[2];
	result.v[2] = matrix.m[2][0] * vector.v[0] + matrix.m[2][1] * vector.v[1] + matrix.m[2][2] * vector.v[2];

	return result;
}

vr::HmdQuaternion_t QuaternionFromAngle(const double& xx, const double& yy, const double& zz, const double& a)
{
	// Here we calculate the sin( theta / 2) once for optimization
	double factor = sin(a / 2.0);

	// Calculate the x, y and z of the quaternion
	double x = xx * factor;
	double y = yy * factor;
	double z = zz * factor;

	// Calcualte the w value by cos( theta / 2 )
	double w = cos(a / 2.0);

	double n = std::sqrt(x * x + y * y + z * z + w * w);
	x /= n;
	y /= n;
	z /= n;
	w /= n;

	vr::HmdQuaternion_t quat = { w,x,y,z };

	return quat;
}
vr::HmdQuaternion_t MultiplyQuaternion(vr::HmdQuaternion_t q, vr::HmdQuaternion_t r) {
	vr::HmdQuaternion_t result;

	result.w = (r.w * q.w - r.x * q.x - r.y * q.y - r.z * q.z);
	result.x = (r.w * q.x + r.x * q.w - r.y * q.z + r.z * q.y);
	result.y = (r.w * q.y + r.x * q.z + r.y * q.w - r.z * q.x);
	result.z = (r.w * q.z - r.x * q.y + r.y * q.x + r.z * q.w);

	return result;
}