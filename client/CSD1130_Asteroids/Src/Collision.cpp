/******************************************************************************/
/*!
\file		Collision.cpp
\author		GWEE BOON XUEN SEAN, g.boonxuensean, 2301326
\par		g.boonxuensean@digipen.edu
\date		08 Feb 2024
\brief		This file contains the function for the collision logic using static and dynamic collision.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/


#include "main.h"

/**************************************************************************/
/*!

	*/
/**************************************************************************/
bool CollisionIntersection_RectRect(const AABB & aabb1,          //Input
									const AEVec2 & vel1,         //Input 
									const AABB & aabb2,          //Input 
									const AEVec2 & vel2,         //Input
									float& firstTimeOfCollision) //Output: the calculated value of tFirst, below, must be returned here
{
	UNREFERENCED_PARAMETER(aabb1);
	UNREFERENCED_PARAMETER(vel1);
	UNREFERENCED_PARAMETER(aabb2);
	UNREFERENCED_PARAMETER(vel2);
	UNREFERENCED_PARAMETER(firstTimeOfCollision);

	// Static Collision
	if (aabb1.min.x > aabb2.max.x || aabb2.min.x > aabb1.max.x || aabb1.max.y < aabb2.min.y || aabb2.max.y < aabb1.min.y) {
		return 0;
	}

	//Dynamic Collision
	AEVec2 Vb;
	Vb.x = vel2.x - vel1.x;
	Vb.y = vel2.y - vel1.y;

	float tFirst = { 0 };
	float tLast = { 0 };
	float dFirst = { 0 };
	float dLast = { 0 };

	if (Vb.x < 0) {
		// Case 1
		if (aabb1.min.x > aabb2.max.x) {
			return 0;
		}
		// Case 4
		if (aabb1.max.x < aabb2.min.x) {
			dFirst = (aabb1.max.x - aabb2.min.x) / Vb.x;
			tFirst = dFirst > tFirst ? dFirst : tFirst;
		}
		if (aabb1.min.x < aabb2.max.x) {
			dLast = (aabb1.min.x - aabb2.max.x) / Vb.x;
			tLast = dLast > tLast ? tLast : dLast;
		}
	}
	else if (Vb.x > 0) {
		// Case 2
		if (aabb1.min.x > aabb2.max.x) {
			dFirst = (aabb1.min.x - aabb2.max.x) / Vb.x;
			tFirst = dFirst > tFirst ? dFirst : tFirst;
		}
		if (aabb1.max.x > aabb2.max.x) {
			dLast = (aabb1.max.x - aabb2.min.x) / Vb.x;
			tLast = dLast > tLast ? tLast : dLast;
		}
		// Case 3
		if (aabb1.max.x < aabb2.min.x) {
			return 0;
		}
	}
	else if (Vb.x == 0) {
		// Case 5
		if (aabb1.max.x < aabb2.min.x) {
			return 0;
		}
		else if (aabb1.min.x > aabb2.max.x) {
			return 0;
		}
	}
	// Case 6
	if (tFirst > tLast) {
		return 0;
	}

	// For Y Axis
	if (Vb.y < 0) {
		// Case 1
		if (aabb1.min.y > aabb2.max.y) {
			return 0;
		}
		// Case 4
		if (aabb1.max.y < aabb2.min.y) {
			dFirst = (aabb1.max.y - aabb2.min.y) / Vb.y;
			tFirst = dFirst > tFirst ? dFirst : tFirst;
		}
		if (aabb1.min.y < aabb2.max.y) {
			dLast = (aabb1.min.y - aabb2.max.y) / Vb.y;
			tLast = dLast > tLast ? tLast : dLast;
		}
	}
	else if (Vb.y > 0) {
		// Case 2
		if (aabb1.min.y > aabb2.max.y) {
			dFirst = (aabb1.min.y - aabb2.max.y) / Vb.y;
			tFirst = dFirst > tFirst ? dFirst : tFirst;
		}
		if (aabb1.max.y > aabb2.max.y) {
			dLast = (aabb1.max.y - aabb2.min.y) / Vb.y;
			tLast = dLast > tLast ? tLast : dLast;
		}
		// Case 3
		if (aabb1.max.y < aabb2.min.y) {
			return 0;
		}
	}
	else if (Vb.y == 0) {
		// Case 5
		if (aabb1.max.y < aabb2.min.y) {
			return 0;
		}
		else if (aabb1.min.y > aabb2.max.y) {
			return 0;
		}
	}
	// Case 6
	if (tFirst > tLast) {
		return 0;
	}

	return 1;

	/*
	Implement the collision intersection over here.

	The steps are:	
	Step 1: Check for static collision detection between rectangles (static: before moving). 
				If the check returns no overlap, you continue with the dynamic collision test
					with the following next steps 2 to 5 (dynamic: with velocities).
				Otherwise you return collision is true, and you stop.
	
	Step 2: Initialize and calculate the new velocity of Vb
			tFirst = 0  //tFirst variable is commonly used for both the x-axis and y-axis
			tLast = dt  //tLast variable is commonly used for both the x-axis and y-axis

	Step 3: Working with one dimension (x-axis).
			if(Vb < 0)
				case 1
				case 4
			else if(Vb > 0)
				case 2
				case 3
			else //(Vb == 0)
				case 5

			case 6

	Step 4: Repeat step 3 on the y-axis

	Step 5: Return true: the rectangles intersect

	*/
}