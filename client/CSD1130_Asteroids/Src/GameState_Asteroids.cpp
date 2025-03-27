/******************************************************************************/
/*!
\file		GameState_Asteroids.cpp
\author		GWEE BOON XUEN SEAN, g.boonxuensean, 2301326
\par		g.boonxuensean@digipen.edu
\date		08 Feb 2024
\brief		This file contains the entire game flow to run the game asteroids.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "main.h"

/******************************************************************************/
/*!
	Defines
*/
/******************************************************************************/
const unsigned int	GAME_OBJ_NUM_MAX		= 32;			// The total number of different objects (Shapes)
const unsigned int	GAME_OBJ_INST_NUM_MAX	= 2048;			// The total number of different game object instances


const unsigned int	SHIP_INITIAL_NUM		= 3;			// initial number of ship lives
const float			SHIP_SCALE_X			= 16.0f;		// ship scale x
const float			SHIP_SCALE_Y			= 16.0f;		// ship scale y
const float			BULLET_SCALE_X			= 20.0f;		// bullet scale x
const float			BULLET_SCALE_Y			= 3.0f;			// bullet scale y
const float			ASTEROID_MIN_SCALE_X	= 10.0f;		// asteroid minimum scale x
const float			ASTEROID_MAX_SCALE_X	= 60.0f;		// asteroid maximum scale x
const float			ASTEROID_MIN_SCALE_Y	= 10.0f;		// asteroid minimum scale y
const float			ASTEROID_MAX_SCALE_Y	= 60.0f;		// asteroid maximum scale y

const float			WALL_SCALE_X			= 64.0f;		// wall scale x
const float			WALL_SCALE_Y			= 164.0f;		// wall scale y

const float			SHIP_ACCEL_FORWARD		= 100.0f;		// ship forward acceleration (in m/s^2)
const float			SHIP_ACCEL_BACKWARD		= 100.0f;		// ship backward acceleration (in m/s^2)
const float			SHIP_ROT_SPEED			= (2.0f * PI);	// ship rotation speed (degree/second)

const float			BULLET_SPEED			= 400.0f;		// bullet speed (m/s)

const float         BOUNDING_RECT_SIZE      = 1.0f;         // this is the normalized bounding rectangle (width and height) sizes - AABB collision data

const unsigned int	WINNING_SCORE			= 5000;			// Score needed to end game
// -----------------------------------------------------------------------------
enum TYPE
{
	// list of game object types
	TYPE_SHIP = 0, 
	TYPE_BULLET,
	TYPE_WALL,
	TYPE_ASTEROID,

	TYPE_NUM
};

enum WEAPON_TYPE
{
	REGULAR_SHOT = 0,
	DOUBLE_SHOT,
	TRIPLE_SHOT,
};

// -----------------------------------------------------------------------------
// object flag definition

const unsigned long FLAG_ACTIVE				= 0x00000001;

/******************************************************************************/
/*!
	Struct/Class Definitions
*/
/******************************************************************************/

//Game object structure
struct GameObj
{
	unsigned long		type;		// object type
	AEGfxVertexList *	pMesh;		// This will hold the triangles which will form the shape of the object
};

// ---------------------------------------------------------------------------

//Game object instance structure
struct GameObjInst
{
	GameObj *			pObject;	// pointer to the 'original' shape
	unsigned long		flag;		// bit flag or-ed together
	AEVec2				scale;		// scaling value of the object instance
	AEVec2				posCurr;	// object current position

	AEVec2				posPrev;	// object previous position -> it's the position calculated in the previous loop

	AEVec2				velCurr;	// object current velocity
	float				dirCurr;	// object current direction
	AABB				boundingBox;// object bouding box that encapsulates the object
	AEMtx33				transform;	// object transformation matrix: Each frame, 
									// calculate the object instance's transformation matrix and save it here

};

/******************************************************************************/
/*!
	Static Variables
*/
/******************************************************************************/

// list of original object
static GameObj				sGameObjList[GAME_OBJ_NUM_MAX];				// Each element in this array represents a unique game object (shape)
static unsigned long		sGameObjNum;								// The number of defined game objects

// list of object instances
static GameObjInst			sGameObjInstList[GAME_OBJ_INST_NUM_MAX];	// Each element in this array represents a unique game object instance (sprite)
static unsigned long		sGameObjInstNum;							// The number of used game object instances

// pointer to the ship object
static GameObjInst *		spShip;										// Pointer to the "Ship" game object instance

// pointer to the wall object
static GameObjInst *		spWall;										// Pointer to the "Wall" game object instance

// number of ship available (lives 0 = game over)
static long					sShipLives;									// The number of lives left

// the score = number of asteroid destroyed
static unsigned long		sScore;										// Current score

static bool					onValueChange;								// On Value Change

static bool					isGameOver;									// Is Game Over?

static unsigned int			shotSelected;								// Current type of shooting style is selected

static double				lastShotTime;								// Time elapsed since last shot

static s8					pFont;										// Font

static char					pTextScore[1024];							// Store score text so it doesn't get cleared

static char					pTextHealth[1024];							// Store health text so it doesn't get cleared

static char					pTextGame[1024];							// Store gameover text so it doesn't get cleared

static char					pTextShots[1024];							// Store shooting style text so it doesn't get cleared
// ---------------------------------------------------------------------------

// functions to create/destroy a game object instance
GameObjInst *		gameObjInstCreate (unsigned long type, AEVec2* scale,
											   AEVec2 * pPos, AEVec2 * pVel, float dir);
void				gameObjInstDestroy(GameObjInst * pInst);

void				Helper_Create_Random_Asteroids();

void				Helper_Wall_Collision();


/******************************************************************************/
/*!
	"Load" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsLoad(void)
{
	// zero the game object array
	memset(sGameObjList, 0, sizeof(GameObj) * GAME_OBJ_NUM_MAX);
	// No game objects (shapes) at this point
	sGameObjNum = 0;

	// zero the game object instance array
	memset(sGameObjInstList, 0, sizeof(GameObjInst) * GAME_OBJ_INST_NUM_MAX);
	// No game object instances (sprites) at this point
	sGameObjInstNum = 0;

	// The ship object instance hasn't been created yet, so this "spShip" pointer is initialized to 0
	spShip = nullptr;

	// load/create the mesh data (game objects / Shapes)
	GameObj * pObj;

	// =====================
	// create the ship shape
	// =====================

	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_SHIP;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,  0.5f, 0xFFFF0000, 0.0f, 0.0f, 
		-0.5f, -0.5f, 0xFFFF0000, 0.0f, 0.0f,
		 0.5f,  0.0f, 0xFFFFFFFF, 0.0f, 0.0f );  

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");


	// =======================
	// create the bullet shape
	// =======================

	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_BULLET;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xcce043fb, 0.0f, 0.0f,
		 0.5f, 0.5f, 0xcce043fb, 0.0f, 0.0f,
		-0.5f, 0.5f, 0xcce043fb, 0.0f, 0.0f);
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xcce043fb, 0.0f, 0.0f,
		 0.5f, -0.5f, 0xcce043fb, 0.0f, 0.0f,
		 0.5f, 0.5f, 0xcce043fb, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");


	// =========================
	// create the wall shape
	// =========================

	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_WALL;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f,
		-0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f);
	AEGfxTriAdd(
		-0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, -0.5f, 0x6600FF00, 0.0f, 0.0f,
		0.5f, 0.5f, 0x6600FF00, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");	


	// =========================
	// create the asteroid shape
	// =========================

	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_ASTEROID;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xff949494, 0.0f, 0.0f,
		0.5f, 0.5f, 0xff949494, 0.0f, 0.0f,
		-0.5f, 0.5f, 0xff949494, 0.0f, 0.0f);
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xff949494, 0.0f, 0.0f,
		0.5f, -0.5f, 0xff949494, 0.0f, 0.0f,
		0.5f, 0.5f, 0xff949494, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	// Load the font here 
	pFont = AEGfxCreateFont("../Resources/Fonts/Arial Italic.ttf", 32);

}

/******************************************************************************/
/*!
	"Initialize" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsInit(void)
{
	// create the main ship
	AEVec2 scale;
	AEVec2Set(&scale, SHIP_SCALE_X, SHIP_SCALE_Y);
	spShip = gameObjInstCreate(TYPE_SHIP, &scale, nullptr, nullptr, 0.0f);
	AE_ASSERT(spShip);


	// create the initial 4 asteroids instances using the "gameObjInstCreate" function
	AEVec2 pos, vel;

	// Create random asteroid shapes based on the min and max scale
	float randSizeX = 0;
	float randSizeY = 0;
	randSizeX = ASTEROID_MIN_SCALE_X + rand() % (int)(ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X);
	randSizeY = ASTEROID_MIN_SCALE_Y + rand() % (int)(ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y);


	//Asteroid 1
	pos.x = 90.0f;		pos.y = -220.0f;
	vel.x = -60.0f;		vel.y = -30.0f;
	AEVec2Set(&scale, randSizeX, randSizeY);
	gameObjInstCreate(TYPE_ASTEROID, &scale, &pos, &vel, 0.0f);

	randSizeX = ASTEROID_MIN_SCALE_X + rand() % (int)(ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X);
	randSizeY = ASTEROID_MIN_SCALE_Y + rand() % (int)(ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y);


	//Asteroid 2
	pos.x = -260.0f;	pos.y = -250.0f;
	vel.x = 39.0f;		vel.y = -130.0f;
	AEVec2Set(&scale, randSizeX, randSizeY);
	gameObjInstCreate(TYPE_ASTEROID, &scale, &pos, &vel, 0.0f);

	randSizeX = ASTEROID_MIN_SCALE_X + rand() % (int)(ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X);
	randSizeY = ASTEROID_MIN_SCALE_Y + rand() % (int)(ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y);


	//Asteroid 3
	pos.x = 220.0f;		pos.y = -190.0f;
	vel.x = 10.0f;		vel.y = 130.0f;
	AEVec2Set(&scale, randSizeX, randSizeY);
	gameObjInstCreate(TYPE_ASTEROID, &scale, &pos, &vel, 0.0f);

	randSizeX = ASTEROID_MIN_SCALE_X + rand() % (int)(ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X);
	randSizeY = ASTEROID_MIN_SCALE_Y + rand() % (int)(ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y);


	//Asteroid 4
	pos.x = 60.0f;		pos.y = 110.0f;
	vel.x = 120.0f;		vel.y = -60.0f;
	AEVec2Set(&scale, randSizeX, randSizeY);
	gameObjInstCreate(TYPE_ASTEROID, &scale, &pos, &vel, 0.0f);

	// create the static wall
	AEVec2Set(&scale, WALL_SCALE_X, WALL_SCALE_Y);
	AEVec2 position;
	AEVec2Set(&position, 300.0f, 150.0f);
	spWall = gameObjInstCreate(TYPE_WALL, &scale, &position, nullptr, 0.0f);
	AE_ASSERT(spWall);


	// reset the score and the number of ships
	sScore      = 0;
	sShipLives  = SHIP_INITIAL_NUM;
	onValueChange = true;
	isGameOver = false;

	// set shooting style to regular
	shotSelected = REGULAR_SHOT;
	lastShotTime = 0;

	// Initalise as empty
	sprintf_s(pTextScore, "");
	sprintf_s(pTextHealth, "");
	sprintf_s(pTextGame, "");
	sprintf_s(pTextShots, "");
}

/******************************************************************************/
/*!
	"Update" function of this state
*/
/******************************************************************************/
void GameStateAsteroidsUpdate(void)
{
	// =========================================================
	// update according to input
	// =========================================================

	// This input handling moves the ship without any velocity nor acceleration
	// It should be changed when implementing the Asteroids project
	//
	// Updating the velocity and position according to acceleration is 
	// done by using the following:
	// Pos1 = 1/2 * a*t*t + v0*t + Pos0
	//
	// In our case we need to divide the previous equation into two parts in order 
	// to have control over the velocity and that is done by:
	//
	// v1 = a*t + v0		//This is done when the UP or DOWN key is pressed 
	// Pos1 = v1*t + Pos0
	if (!isGameOver) {
		// Move up
		if (AEInputCheckCurr(AEVK_UP))
		{
			// Find the velocity according to the acceleration
			AEVec2 vel;
			AEVec2Set(&vel, SHIP_ACCEL_FORWARD * cosf(spShip->dirCurr), SHIP_ACCEL_FORWARD * sinf(spShip->dirCurr));
			AEVec2Scale(&vel, &vel, g_dt);
			AEVec2Add(&spShip->velCurr, &spShip->velCurr, &vel);

			// Add velocity cap
			AEVec2Scale(&spShip->velCurr, &spShip->velCurr, 0.99f);
		}
		// Move down
		if (AEInputCheckCurr(AEVK_DOWN))
		{

			// Find the velocity according to the decceleration
			AEVec2 vel;
			AEVec2Set(&vel, SHIP_ACCEL_FORWARD * -cosf(spShip->dirCurr), SHIP_ACCEL_FORWARD * -sinf(spShip->dirCurr));
			AEVec2Scale(&vel, &vel, g_dt);
			AEVec2Add(&spShip->velCurr, &spShip->velCurr, &vel);

			//Add velocity cap
			AEVec2Scale(&spShip->velCurr, &spShip->velCurr, 0.99f);
		}

		// Rotate left
		if (AEInputCheckCurr(AEVK_LEFT))
		{
			spShip->dirCurr += SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime());
			spShip->dirCurr = AEWrap(spShip->dirCurr, -PI, PI);
		}
		// Rotate right
		if (AEInputCheckCurr(AEVK_RIGHT))
		{
			spShip->dirCurr -= SHIP_ROT_SPEED * (float)(AEFrameRateControllerGetFrameTime());
			spShip->dirCurr = AEWrap(spShip->dirCurr, -PI, PI);
		}

		//Swap shooting style
		if (AEInputCheckTriggered(AEVK_1)) {
			shotSelected = REGULAR_SHOT;
			printf("REGULAR SHOTS SELECTED\n");
		}
		if (AEInputCheckTriggered(AEVK_2)) {
			shotSelected = DOUBLE_SHOT;
			printf("DOUBLE SHOT SELECTED\n");
		}
		if (AEInputCheckTriggered(AEVK_3)) {
			shotSelected = TRIPLE_SHOT;
			printf("TRIPLE SHOT SELECTED\n");
		}

		// Shoot a bullet if space is triggered (Create a new object instance)
		if (AEInputCheckTriggered(AEVK_SPACE))
		{
			AEVec2 vel;
			AEVec2 scale;
			double currentTime = AEGetTime(nullptr);

	
			// Get the bullet's direction according to the ship's direction
			// Set the velocity
			AEVec2Set(&vel, cosf(spShip->dirCurr) * BULLET_SPEED, sinf(spShip->dirCurr) * BULLET_SPEED);
			// Create an instance, based on BULLET_SCALE_X and BULLET_SCALE_Y
			AEVec2Set(&scale, BULLET_SCALE_X, BULLET_SCALE_Y);
			
			// Switch case to shoot based on shooting style
			switch (shotSelected) {
				case REGULAR_SHOT:
					// Create 1 shots

					gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr);
					break;
				case DOUBLE_SHOT:
					// Create 2 shots
					if ((currentTime - lastShotTime) >= 0.4f) {
						AEVec2Set(&vel, cosf(spShip->dirCurr + 0.25f) * BULLET_SPEED, sinf(spShip->dirCurr + 0.25f) * BULLET_SPEED);
						gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr + 0.25f);
						AEVec2Set(&vel, cosf(spShip->dirCurr - 0.25f) * BULLET_SPEED, sinf(spShip->dirCurr - 0.25f) * BULLET_SPEED);
						gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr - 0.25f);
						lastShotTime = currentTime;
					}
				
					break;
				case TRIPLE_SHOT:
					// Create 3 shots
					if ((currentTime - lastShotTime) >= 0.8f) {
						gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr);
						AEVec2Set(&vel, cosf(spShip->dirCurr + 0.35f) * BULLET_SPEED, sinf(spShip->dirCurr + 0.35f) * BULLET_SPEED);
						gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr + 0.35f);
						AEVec2Set(&vel, cosf(spShip->dirCurr - 0.35f) * BULLET_SPEED, sinf(spShip->dirCurr - 0.35f) * BULLET_SPEED);
						gameObjInstCreate(TYPE_BULLET, &scale, &spShip->posCurr, &vel, spShip->dirCurr - 0.35f);
						lastShotTime = currentTime;
					}
					break;
				default:
					break;
			}
			
		}
	}
	else {
		// Restart game if R is pressed
		if (AEInputCheckTriggered(AEVK_R)) {
			gGameStateNext = GS_RESTART;
		}
	}




	// ======================================================================
	// Save previous positions
	//  -- For all instances
	// [DO NOT UPDATE THIS PARAGRAPH'S CODE]
	// ======================================================================
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst = sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		pInst->posPrev.x = pInst->posCurr.x;
		pInst->posPrev.y = pInst->posCurr.y;
	}






	// ======================================================================
	// update physics of all active game object instances
	//  -- Calculate the AABB bounding rectangle of the active instance, using the starting position:
	//		boundingRect_min = -(BOUNDING_RECT_SIZE/2.0f) * instance->scale + instance->posPrev
	//		boundingRect_max = +(BOUNDING_RECT_SIZE/2.0f) * instance->scale + instance->posPrev
	//
	//	-- New position of the active instance is updated here with the velocity calculated earlier
	// ======================================================================

	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst = sGameObjInstList + i;
		
		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		// Set bounding box 
		pInst->boundingBox.min.x = -(BOUNDING_RECT_SIZE / 2.0f) * pInst->scale.x + pInst->posPrev.x;
		pInst->boundingBox.min.y = -(BOUNDING_RECT_SIZE / 2.0f) * pInst->scale.y + pInst->posPrev.y;
		pInst->boundingBox.max.x = (BOUNDING_RECT_SIZE / 2.0f) * pInst->scale.x + pInst->posPrev.x;
		pInst->boundingBox.max.y = (BOUNDING_RECT_SIZE / 2.0f) * pInst->scale.y + pInst->posPrev.y;

		if (pInst->pObject->type == TYPE_SHIP) // Move ship each game loop
		{
			AEVec2 acc;
			AEVec2Set(&acc, pInst->velCurr.x * g_dt, pInst->velCurr.y * g_dt);
			AEVec2Add(&pInst->posCurr, &acc, &pInst->posCurr);
		}
		if (pInst->pObject->type == TYPE_BULLET) // Move bullet each game loop
		{
			AEVec2 vel;

			// Define a maximum distance threshold
			AEVec2Set(&vel, pInst->velCurr.x* g_dt, pInst->velCurr.y* g_dt);
			AEVec2Add(&pInst->posCurr, &vel, &pInst->posCurr);

		}
		if (pInst->pObject->type == TYPE_ASTEROID) { // Move asteroids each game loop
			AEVec2 vel;
			AEVec2Set(&vel, pInst->velCurr.x * g_dt, pInst->velCurr.y * g_dt);
			AEVec2Add(&pInst->posCurr, &vel, &pInst->posCurr);
		}
	}




	// ======================================================================
	// check for dynamic-static collisions (one case only: Ship vs Wall)
	// [DO NOT UPDATE THIS PARAGRAPH'S CODE]
	// ======================================================================
	Helper_Wall_Collision();





	// ======================================================================
	// check for dynamic-dynamic collisions
	// ======================================================================
	if (!isGameOver) {
		for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
		{
			GameObjInst* pInst = sGameObjInstList + i;


			// skip non-active object
			if ((pInst->flag & FLAG_ACTIVE) == 0)
				continue;

			// Check for all asteroids
			if (pInst->pObject->type == TYPE_ASTEROID) {
				for (unsigned long j = 0; j < GAME_OBJ_INST_NUM_MAX; j++)
				{
					GameObjInst* pInst2 = sGameObjInstList + j;

					float firstTimeOfCollision = 0.0f;
					// skip non-active object
					if ((pInst2->flag & FLAG_ACTIVE) == 0 || pInst2->pObject->type == TYPE_ASTEROID)
						continue;

					// Compare asteroids with types
					if (pInst2->pObject->type == TYPE_SHIP) {
						// if asteroids collide with ship
						if (CollisionIntersection_RectRect(pInst->boundingBox, pInst->velCurr, pInst2->boundingBox, pInst2->velCurr, firstTimeOfCollision)) {
							// Destroy asteroid
							pInst->flag = 0;

							// Reset ship pos
							pInst2->posCurr.x = 0.0f;
							pInst2->posCurr.y = 0.0f;

							// Reset ship vel
							pInst2->velCurr.x = 0.0f;
							pInst2->velCurr.y = 0.0f;

							// Deduct ship's lives
							sShipLives--;
							onValueChange = true;

							Helper_Create_Random_Asteroids();
						}

					}
					// if asteroids collide with bullet
					if (pInst2->pObject->type == TYPE_BULLET) {
						if (CollisionIntersection_RectRect(pInst->boundingBox, pInst->velCurr, pInst2->boundingBox, pInst2->velCurr, firstTimeOfCollision)) {
							// Destroy bullet and asteroid
							pInst2->flag = 0;
							pInst->flag = 0;

							// Increment score
							sScore += 100;
							onValueChange = true;

							// Create an asteroid
							Helper_Create_Random_Asteroids();
							// Randomly chooses to create another asteroid
							if (rand() % 2) {
								Helper_Create_Random_Asteroids();
							}
						}

					}
				}
			}
		}
	}

	// ===================================================================
	// update active game object instances
	// Example:
	//		-- Wrap specific object instances around the world (Needed for the assignment)
	//		-- Removing the bullets as they go out of bounds (Needed for the assignment)
	//		-- If you have a homing missile for example, compute its new orientation 
	//			(Homing missiles are not required for the Asteroids project)
	//		-- Update a particle effect (Not required for the Asteroids project)
	// ===================================================================
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;
		
		// check if the object is a ship
		if (pInst->pObject->type == TYPE_SHIP)
		{
			// Wrap the ship from one end of the screen to the other
			pInst->posCurr.x = AEWrap(pInst->posCurr.x, AEGfxGetWinMinX() - SHIP_SCALE_X, 
														AEGfxGetWinMaxX() + SHIP_SCALE_X);
			pInst->posCurr.y = AEWrap(pInst->posCurr.y, AEGfxGetWinMinY() - SHIP_SCALE_Y,
														AEGfxGetWinMaxY() + SHIP_SCALE_Y);
		}

		// Wrap asteroids here
		if (pInst->pObject->type == TYPE_ASTEROID)
		{
			// Wrap the ship from one end of the screen to the other
			pInst->posCurr.x = AEWrap(pInst->posCurr.x, AEGfxGetWinMinX() - ASTEROID_MIN_SCALE_X,
				AEGfxGetWinMaxX() + ASTEROID_MAX_SCALE_X);
			pInst->posCurr.y = AEWrap(pInst->posCurr.y, AEGfxGetWinMinY() - ASTEROID_MIN_SCALE_Y,
				AEGfxGetWinMaxY() + ASTEROID_MAX_SCALE_Y);
		}

		// Remove bullets that go out of bounds
		if (pInst->pObject->type == TYPE_BULLET)
		{
			// When bullet goes out of bounds
			if (pInst->posCurr.x > AEGfxGetWindowWidth() || pInst->posCurr.y > AEGfxGetWindowHeight() || pInst->posCurr.y < 0 - AEGfxGetWindowHeight() || pInst->posCurr.x < 0 - AEGfxGetWindowWidth()) {
				pInst->flag = 0;
			}
		}
	}



	

	// =====================================================================
	// calculate the matrix for all objects
	// =====================================================================

	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;
		AEMtx33		 trans{ 0 }, rot{ 0 }, scale{ 0 };

		UNREFERENCED_PARAMETER(trans);
		UNREFERENCED_PARAMETER(rot);
		UNREFERENCED_PARAMETER(scale);

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;

		// Compute the scaling matrix
		AEMtx33Scale(&scale, pInst->scale.x, pInst->scale.y);
		// Compute the rotation matrix 
		AEMtx33Rot(&rot, pInst->dirCurr);
		// Compute the translation matrix
		AEMtx33Trans(&trans, pInst->posCurr.x, pInst->posCurr.y);
		// Concatenate the 3 matrix in the correct order in the object instance's "transform" matrix
		AEMtx33Concat(&pInst->transform, &rot, &scale);
		AEMtx33Concat(&pInst->transform, &trans, &pInst->transform);
	}
}

/******************************************************************************/
/*!
	
*/
/******************************************************************************/
void GameStateAsteroidsDraw(void)
{
	char strBuffer[1024];

	
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxTextureSet(NULL, 0, 0);


	// Set blend mode to AE_GFX_BM_BLEND
	// This will allow transparency.
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);


	// draw all object instances in the list
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;

		// skip non-active object
		if ((pInst->flag & FLAG_ACTIVE) == 0)
			continue;
		
		// Set the current object instance's transform matrix using "AEGfxSetTransform"
		AEGfxSetTransform(pInst->transform.m);
		// Draw the shape used by the current object instance using "AEGfxMeshDraw"
		AEGfxMeshDraw(pInst->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
	}

	//You can replace this condition/variable by your own data.
	//The idea is to display any of these variables/strings whenever a change in their value happens

	if(onValueChange)
	{
		// Print Score
		sprintf_s(pTextScore, "Score: %d", sScore);
		sprintf_s(strBuffer, "Score: %d", sScore);
		printf("%s \n", strBuffer);

		// Print ship health
		sprintf_s(pTextHealth, "Ship Left: %d", sShipLives >= 0 ? sShipLives : 0);
		sprintf_s(strBuffer, "Ship Left: %d", sShipLives >= 0 ? sShipLives : 0);
		printf("%s \n", strBuffer);

		// display the game over message
		if (sShipLives < 0)
		{
			isGameOver = true;
			sprintf_s(pTextGame, "YOU LOSE!");

			printf("       GAME OVER       \n");
			printf("PLAY AGAIN? PRESS \"R\" \n");
		}

		// display winning message when score is equals winning score
		if (sScore >= WINNING_SCORE) {
			isGameOver = true;
			sprintf_s(pTextGame, "YOU WIN!");
			printf("       YOU ROCK!       \n");
			printf("PLAY AGAIN? PRESS \"R\" \n");
		}

		onValueChange = false;
	}

	// Check shot selected
	switch (shotSelected) {
	case REGULAR_SHOT:
		// print regular shots
		sprintf_s(pTextShots, "REGULAR SHOTS");
		break;
	case DOUBLE_SHOT:
		// Print double shots
		sprintf_s(pTextShots, "DOUBLE SHOT");
		break;
	case TRIPLE_SHOT:
		// print triple shot
		sprintf_s(pTextShots, "TRIPLE SHOT");
		break;
	default:
		break;
	}

	// Print text to screen
	AEGfxPrint(pFont, pTextScore, -0.9f, -0.9f, 1, 1, 1, 1, 1);
	AEGfxPrint(pFont, pTextHealth, 0.5f, -0.9f, 1, 1, 1, 1, 1);
	AEGfxPrint(pFont, pTextGame, -0.2f, 0.0f, 1, 1, 1, 1, 1);
	AEGfxPrint(pFont, pTextShots, -0.98f, 0.9f, 0.5, 1, 1, 1, 1);
}

/******************************************************************************/
/*!
	
*/
/******************************************************************************/
void GameStateAsteroidsFree(void)
{
	// kill all object instances in the array using "gameObjInstDestroy"
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst = sGameObjInstList + i;

		gameObjInstDestroy(pInst);
	}


}

/******************************************************************************/
/*!
	
*/
/******************************************************************************/
void GameStateAsteroidsUnload(void)
{
	// free all mesh data (shapes) of each object using "AEGfxTriFree"
	for (unsigned long i = 0; i < sGameObjNum; i++)
	
	{
	GameObj* pInst = sGameObjList + i;

		AEGfxMeshFree(pInst->pMesh);
	
	}
	// Free font
	AEGfxDestroyFont(pFont);
}

/******************************************************************************/
/*!
	
*/
/******************************************************************************/
GameObjInst * gameObjInstCreate(unsigned long type, 
							   AEVec2 * scale,
							   AEVec2 * pPos, 
							   AEVec2 * pVel, 
							   float dir)
{
	AEVec2 zero;
	AEVec2Zero(&zero);

	AE_ASSERT_PARM(type < sGameObjNum);
	
	// loop through the object instance list to find a non-used object instance
	for (unsigned long i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst * pInst = sGameObjInstList + i;

		// check if current instance is not used
		if (pInst->flag == 0)
		{
			// it is not used => use it to create the new instance
			pInst->pObject	= sGameObjList + type;
			pInst->flag		= FLAG_ACTIVE;
			pInst->scale	= *scale;
			pInst->posCurr	= pPos ? *pPos : zero;
			pInst->velCurr	= pVel ? *pVel : zero;
			pInst->dirCurr	= dir;
			
			// return the newly created instance
			return pInst;
		}
	}

	// cannot find empty slot => return 0
	return 0;
}

/******************************************************************************/
/*!
	
*/
/******************************************************************************/
void gameObjInstDestroy(GameObjInst * pInst)
{
	// if instance is destroyed before, just return
	if (pInst->flag == 0)
		return;

	// zero out the flag
	pInst->flag = 0;
}

/******************************************************************************/
/*!
    check for collision between Ship and Wall and apply physics response on the Ship
		-- Apply collision response only on the "Ship" as we consider the "Wall" object is always stationary
		-- We'll check collision only when the ship is moving towards the wall!
	[DO NOT UPDATE THIS PARAGRAPH'S CODE]
*/
/******************************************************************************/
void Helper_Wall_Collision()
{
	//calculate the vectors between the previous position of the ship and the boundary of wall
	AEVec2 vec1;
	vec1.x = spShip->posPrev.x - spWall->boundingBox.min.x;
	vec1.y = spShip->posPrev.y - spWall->boundingBox.min.y;
	AEVec2 vec2;
	vec2.x = 0.0f;
	vec2.y = -1.0f;
	AEVec2 vec3;
	vec3.x = spShip->posPrev.x - spWall->boundingBox.max.x;
	vec3.y = spShip->posPrev.y - spWall->boundingBox.max.y;
	AEVec2 vec4;
	vec4.x = 1.0f;
	vec4.y = 0.0f;
	AEVec2 vec5;
	vec5.x = spShip->posPrev.x - spWall->boundingBox.max.x;
	vec5.y = spShip->posPrev.y - spWall->boundingBox.max.y;
	AEVec2 vec6;
	vec6.x = 0.0f;
	vec6.y = 1.0f;
	AEVec2 vec7;
	vec7.x = spShip->posPrev.x - spWall->boundingBox.min.x;
	vec7.y = spShip->posPrev.y - spWall->boundingBox.min.y;
	AEVec2 vec8;
	vec8.x = -1.0f;
	vec8.y = 0.0f;
	if (
		(AEVec2DotProduct(&vec1, &vec2) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec2) <= 0.0f) ||
		(AEVec2DotProduct(&vec3, &vec4) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec4) <= 0.0f) ||
		(AEVec2DotProduct(&vec5, &vec6) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec6) <= 0.0f) ||
		(AEVec2DotProduct(&vec7, &vec8) >= 0.0f) && (AEVec2DotProduct(&spShip->velCurr, &vec8) <= 0.0f)
		)
	{
		float firstTimeOfCollision = 0.0f;
		if (CollisionIntersection_RectRect(spShip->boundingBox,
			spShip->velCurr,
			spWall->boundingBox,
			spWall->velCurr,
			firstTimeOfCollision))
		{
			//re-calculating the new position based on the collision's intersection time
			spShip->posCurr.x = spShip->velCurr.x * (float)firstTimeOfCollision + spShip->posPrev.x;
			spShip->posCurr.y = spShip->velCurr.y * (float)firstTimeOfCollision + spShip->posPrev.y;

			//reset ship velocity
			spShip->velCurr.x = 0.0f;
			spShip->velCurr.y = 0.0f;
		}
	}
}

void Helper_Create_Random_Asteroids()
{
	AEVec2 pos, vel, scale;
	float randSizeX = 0;
	float randSizeY = 0;

	// Randomise size
	randSizeX = ASTEROID_MIN_SCALE_X + rand() % (int)(ASTEROID_MAX_SCALE_X - ASTEROID_MIN_SCALE_X);
	randSizeY = ASTEROID_MIN_SCALE_Y + rand() % (int)(ASTEROID_MAX_SCALE_Y - ASTEROID_MIN_SCALE_Y);

	// Generate the position randomly
	pos.x = rand() % (int)(AEGfxGetWindowWidth()) - (float)(AEGfxGetWindowWidth() / 2); pos.y = (float)AEGfxGetWindowHeight() / 2.0f;
	vel.x = rand() % 400 - 200.0f;		vel.y = rand() % 400 - 200.0f;
	AEVec2Set(&scale, randSizeX, randSizeY);
	gameObjInstCreate(TYPE_ASTEROID, &scale, &pos, &vel, 0.0f);
}