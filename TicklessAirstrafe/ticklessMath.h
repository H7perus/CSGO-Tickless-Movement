#include "shared/gamemovement.h"

QAngle m_vecPrevViewAngles;

float normalizeAngleRad(float angle)
{
	angle = fmod(angle, 2 * M_PI);
	if (angle < -M_PI)
	{
		angle += 2 * M_PI;
	}
	if (angle > M_PI)
	{
		angle -= 2 * M_PI;
	}
	return angle;
}

//H7per: I am simply going to let this thing here decide until when it wants to run!
uint8_t integrateConstantAccel(const Vector& velocity, Vector& addedvel, float accel, float start_rotation, float w, float& time)
{
	float time_end = 1.0;
	//5 = ran through, 4 = hit 30u/s limit
	uint8_t returnval = 5;
	//first we want to figure out when we would run into the 30u/s limit.
	

	Vector totalvel = velocity + addedvel;


	float r0 = start_rotation + w * time;
	float start_angle_to_vel = r0 - atan2(totalvel.y, totalvel.x);

	double a_over_w = accel / double(w);

	double R = sqrt(totalvel.Length2DSqr() + a_over_w * (a_over_w - 2 * totalvel.Length2D() * sin(start_angle_to_vel)));


	if (R > 30.00001)
	{
		double atan2_y = totalvel.Length2D() * cos(start_angle_to_vel);
		double atan2_x = -totalvel.Length2D() * sin(start_angle_to_vel) + accel / w;

		double phi = atan2(atan2_y, atan2_x);

		//our function for the dot product we are trying to limit would be R * sin(wt + phi), where t is the time - timestart
		//if we just ran the integral all the way, we would often exceed 30u/s, so we want to figure out if we go above that limit and only run this constant accel integral until then

		int sign_w = (w >= 0) - (w < 0);

		double angle = M_PI + asin(30.0 / R) - sign_w * (phi + (w > 0 ? M_PI : 0));

		angle += (angle > 2 * M_PI) * -2 * M_PI + (angle < 0) * 2 * M_PI;

		double max_angle = 2 * M_PI - acos(double(30.f / R));

		if (angle > max_angle)
		{
			return 5;
		}

		float inb_time = (angle) / abs(w);

		if (inb_time > 0 && inb_time < 1.0 - time)
		{
			time_end = inb_time + time;
			returnval = 4;
		}

	}

	float r1 = start_rotation + time_end * w;

	addedvel += (a_over_w)*Vector(sin(r1) - sin(r0), -cos(r1) + cos(r0), 0);

	float dot = Vector(cos(r1), sin(r1), 0).Dot(velocity + addedvel);
	if (dot > 30.001f)
		Msg("The currentspeed exceeded max in constant accel int: %.2f \n", dot);

	time = time_end;
	return returnval;
}

//H7per: The function assumes that the dot product is already 30!. If it is not, DO NOT USE THIS.
uint8_t integrateConstantDotProductSpeed(const Vector& velocity, Vector& veladded, float accel, float start_rotation, float w, float& time)
{
	//5 = ran through to the end, 2 = acceleration reduced to zero(need recheck to see when we can accel again, if we can at all), 3 = acceleration would have exceeded max accel
	uint8_t returnval = 5;

	float time_end = 1.0;
	float r0 = start_rotation + w * time;
	Vector totalvel = velocity + veladded;

	
	float start_alpha = r0 - atan2(totalvel.y, totalvel.x);

	float b = sin(start_alpha) * w * totalvel.Length2D();

	if (b <= 0)
	{
		//this can happen if we go straight from constant accel to this and the acceleration vector is rotating towards the velocity vector. Its a rare case but needs to be checked for.
		return 1;
	}

	//that 30.f looks like a magic number, mathematically rigorous would be cos(alpha) * velocity.Length(), but this function is only called when that is 30
	float m = 30.f * pow(w, 2);

	//H7per: check when our accel is between 0 and 30. If the required accel is larger than the available accel, we stop and switch to constant accel instead.
	float time_to_zero = -b / m;
	float time_to_accel = -(b - accel) / m;


	//H7per: This assumes that we already know that we need to constrain the accel, i.e. m * t + b < accel. Otherwise we would just run the constant accel integral
	if (time_to_zero < time_end - time && time_to_zero > time)
	{
		time_end = time + time_to_zero;
		returnval = 2;
	}
	if (time_to_accel < time_end - time && time_to_accel > 0)
	{
		time_end = time + time_to_accel;
		returnval = 3;
	}
	float start_to_end = time_end - time;

	float r1 = start_rotation + w * time_end;

	veladded.x += ((m * start_to_end + b) * sin(r1) - b * sin(r0)) / w;
	veladded.x -= m * (-cos(r1) + cos(r0)) / pow(w, 2);

	veladded.y += ((m * start_to_end + b) * -cos(r1) - b * -cos(r0)) / w;
	veladded.y -= m * (-sin(r1) + sin(r0)) / pow(w, 2);
	time = time_end;

	//For debugging purposes.
	{
	float dot = Vector(cos(r1), sin(r1), 0).Dot(velocity + veladded);
	if (dot > 30.01f)
		Msg("The currentspeed exceeded max in constant dot int: %.2f \n", dot);
	}

	return returnval;

}
