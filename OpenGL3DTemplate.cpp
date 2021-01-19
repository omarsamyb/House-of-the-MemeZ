#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

#define GLUT_KEY_ESCAPE 27
#define DEG2RAD(a) (a * 0.0174532925)
#define PI 3.14159265

void loadStaticPositions(int value);

/* Helper Classes */
class Vector3f {
public:
	float x, y, z;

	Vector3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) {
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3f operator+(Vector3f& v) {
		return Vector3f(x + v.x, y + v.y, z + v.z);
	}

	Vector3f operator+(float n) {
		return Vector3f(x + n, y + n, z + n);
	}

	Vector3f operator-(Vector3f& v) {
		return Vector3f(x - v.x, y - v.y, z - v.z);
	}

	Vector3f operator*(float n) {
		return Vector3f(x * n, y * n, z * n);
	}

	Vector3f operator/(float n) {
		return Vector3f(x / n, y / n, z / n);
	}

	Vector3f unit() {
		return *this / sqrt(x * x + y * y + z * z);
	}

	Vector3f cross(Vector3f v) {
		return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
	float dot(Vector3f v) {
		return x * v.x + y * v.y + z * v.z;
	}
	float magnitude() {
		return sqrt(x * x + y * y + z * z);
	}
};
class Camera {
public:
	Vector3f eye, center, up;
	Camera(float eyeX = 0.99f, float eyeY = 0.6f, float eyeZ = 0.99, float centerX = 0.95f, float centerY = 0.6f, float centerZ = 1.5f, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f) {
		eye = Vector3f(eyeX, eyeY, eyeZ);
		center = Vector3f(centerX, centerY, centerZ);
		up = Vector3f(upX, upY, upZ);
	}

	void moveX(float d) {
		Vector3f right = up.cross(center - eye).unit();
		right.y = 0;
		eye = eye + right * d;
		center = center + right * d;
	}

	void moveY(float d) {
		eye = eye + up.unit() * d;
		center = center + up.unit() * d;
	}

	void moveZ(float d) {
		Vector3f view = (center - eye).unit();
		view.y = 0;
		eye = eye + view * d;
		center = center + view * d;
	}

	void moveWithTarget(Vector3f target, float d) {
		eye = eye + target * d;
		//center = center + target * d;
	}

	void rotateX(float a, bool cameraMode) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + up * sin(DEG2RAD(a));
		//up = view.cross(right);
		if (cameraMode)
			center = eye + view;
		else
			eye = center - view;
	}

	void rotateY(float a, bool cameraMode) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + right * sin(DEG2RAD(a));
		right = view.cross(up);
		if (cameraMode)
			center = eye + view;
		else
			eye = center - view;
	}
	void rotateZ(float a, bool cameraMode) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		up = up * cos(DEG2RAD(a)) + right * sin(DEG2RAD(a));
		right = view.cross(up);
		if (cameraMode)
			center = eye + view;
		else
			eye = center - view;
	}

	void switchMode(bool cameraMode, Vector3f playerModelPosition) {
		// FPP to TPP
		if (cameraMode) {
			Vector3f view = (center - eye).unit();
			eye = eye + view * -0.99;
			center = playerModelPosition;
			center.y = playerModelPosition.y + 0.6;
		}
		// TPP to FPP
		else {
			Vector3f view = (center - eye).unit();
			eye = eye + view * 0.99;
		}
	}

	void look() {
		gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z
		);
	}
};

/* Method Definitions */
void Display1(void);
void Display2(void);
void Anim1();
void Anim2();
Vector3f getVector(Vector3f a, Vector3f b);
Vector3f getViewVector();
void Timer(int value);

/* Global Variables & Constants */
// Game //
bool debugMode = true;
bool boss1FightStart = false;
bool boss2FightStart = false;
int thunderCount = rand() % 6;
bool thunderOn = false;
int currentLevel = 0;
int minutes = 0;
int seconds = 0;
bool start = true;
bool controls = false;
bool about = false;

// Window //
int windowPosX = 50;
int windowPosY = 50;
int width = 1280;
int height = 720;

// Models //
Model_3DS houseModel;
Model_3DS treeModel;
Model_3DS dragonModel;
Model_3DS mountainModel;
Model_3DS wallModel;
Model_3DS wall0Model;
Model_3DS playerModel;
Model_3DS gunModel;
Model_3DS boss1Model;
Model_3DS trap1Model;
Model_3DS treeLogModel;
Model_3DS castleModel;

// Textures //
GLTexture groundTex;
GLTexture ground2Tex;
GLTexture skyTex;
GLTexture sky2Tex;
GLTexture gameoverTex;
GLTexture moon1Tex;
GLTexture moon2Tex;
GLTexture trap2Tex;
GLTexture trap3Tex;
GLTexture fireTex;
GLTexture startTex;
GLTexture controlsTex;
GLTexture aboutTex;
GLTexture survivedTex;

// Controls //
// ratio of inc = 4
float moveRate = 0.04;			////////////////////////////////////////////////////
float lookRate = 1.0;
float sensitivity = 0.1;
float rotationX = 0;
bool keyboardBuffer[256];

// Player Model //
static Vector3f playerModelPosition = Vector3f(1, 0, 1);
static Vector3f playerModelRange = Vector3f(0.1, 0.5, 0.1);
Vector3f gunModelPosition = Vector3f(1, 0.3, 1);
float rotationY = 0;
int playerHealth = 10;
float playerHealthY = 400;
float playerHealthDownRate = 300 / 10;
bool flashOn = false;
float playerRotation = 0;
float weaponRotationX = 0;
bool walking = false;

// Boss 1 Model //
static Vector3f boss1ModelPosition = Vector3f(31, 1.5, 37);
static Vector3f boss1ModelRange = Vector3f(0.45, 1.5, 0.45);
int boss1Health = 15;
int bossMoveX = rand() % 2;
int bossMoveZ = rand() % 2;
float boss1HealthX = 980;
float boss1HealthDownRate = 680 / 15;
float boss1MoveRate = 0.01;				////////////////////////////////////////////////////
float boss1Rotation = 180;

// Boss 2 Model //
static Vector3f boss2ModelPosition = Vector3f(30, 2.75, 30.75);
static Vector3f boss2ModelRange = Vector3f(2, 2.75, 2);
int boss2Health = 20;
int bossMoveY = rand() % 2;
float boss2HealthX = 980;
float boss2HealthDownRate = 680 / 20;
float boss2MoveRate = 0.007;				////////////////////////////////////////////////////
float boss2Rotation = -90;

// Traps //
bool triggered = false;
bool trap1 = false;
Vector3f trap1Position = Vector3f(65, 0, 7.75);
Vector3f trap1Range = Vector3f(0.20, 1, 0.20);
Vector3f trap1Direction;
float trap1MoveRate = 0.08;			////////////////////////////////////////////////////
bool trap2 = false;
Vector3f trap2Position = Vector3f(65, 0, 15.25);
Vector3f trap2Range = Vector3f(0.05, 1, 0.75);
float trap2MoveRate = 0.05;			////////////////////////////////////////////////////
bool trap3 = false;
Vector3f trap3Position = Vector3f(65, 0, 16.75);
Vector3f trap3Range = Vector3f(1, 1, 0.75);
float trap3MoveRate = 0.05;			////////////////////////////////////////////////////
float trap3Angle = 0;

// Moon
float moonRadius = 10;
Vector3f moonPosition = Vector3f(0, 50, -60);

// Camera //
Camera camera;
static bool cameraMode = true;	// false -> TPP , true -> FPP
Vector3f savedCameraUp;
Vector3f savedCameraCenter;
Vector3f savedCameraEye;

// Cutscenes
// boss 1 Cutscene
bool boss1Cutscene = false;
bool boss1CutsceneFlag1 = false;
bool boss1CutsceneFinished = false;
Vector3f boss1CutsceneView;
// level 2 starting cutscene
bool level2CutsceneStarted = false;
bool level2CustsceneFinished = false;
bool flashBangShow = false;
bool flashBangHide = false;
int flashBangY1 = 360;
int flashBangY2 = 360;
int flashBangHideRate = 1;

// Bullets //
Vector3f bulletPosition = Vector3f(0, 0, 0);
Vector3f bulletRange = Vector3f(0.08, 0.08, 0.08);
Vector3f bulletDirection;
Vector3f boss1BulletDirection;
Vector3f boss1BulletPosition = Vector3f(0, 0, 0);
Vector3f boss1BulletRange = Vector3f(0.25, 0.25, 0.25);

Vector3f boss2BulletDirection;
Vector3f boss2BulletPosition = Vector3f(0, 0, 0);
Vector3f boss2BulletRange = Vector3f(0.02, 0.02, 0.02);

float playerBulletSpeed = 0.9;		////////////////////////////////////////////////////
float bossBulletSpeed = 0.16;		////////////////////////////////////////////////////
float bossBulletAngle = 0;
bool playerIsHit = false;
bool bossIsHit = false;

// Collision Detection //
std::unordered_map<std::string, Vector3f> objectPositions;
std::unordered_map<std::string, Vector3f> objectRanges;

/* Helpers */
/* Game */
void reset(int value) {
	// Reset ( New game or player died )
	if (value == 0) {
		mciSendString(TEXT("stop survived"), NULL, 0, NULL);
		mciSendString(TEXT("close survived"), NULL, 0, NULL);
		mciSendString(TEXT("stop background"), NULL, 0, NULL);
		mciSendString(TEXT("close background"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/background.wav\" alias background"), NULL, 0, NULL);
		mciSendString(TEXT("play background"), NULL, 0, NULL);
		currentLevel = 1;
		moveRate = 0.04;
		thunderOn = false;
		boss1FightStart = false;
		boss2FightStart = false;
		camera.eye = Vector3f(0.99, 0.6, 0.99);
		camera.center = Vector3f(0.95, 0.6, 1.5);
		rotationX = 0;
		rotationY = 0;
		playerModelPosition = Vector3f(1, 0, 1);
		playerHealth = 10;
		playerHealthY = 400;
		flashOn = false;
		playerRotation = 0;
		weaponRotationX = 0;
		walking = false;
		boss1ModelPosition = Vector3f(31, 1.5, 37);
		boss1Health = 15;
		boss1HealthX = 980;
		boss1Rotation = 180;
		boss2ModelPosition = Vector3f(30, 2.75, 30.75);
		boss2Health = 20;
		boss2HealthX = 980;
		boss2Rotation = -90;
		triggered = false;
		trap1 = false;
		trap1Position = Vector3f(65, 0, 7.75);
		trap2 = false;
		trap2Position = Vector3f(65, 0, 15.25);
		trap3 = false;
		trap3Position = Vector3f(65, 0, 16.75);
		trap3Angle = 0;
		moonRadius = 10;
		cameraMode = true;
		boss1Cutscene = false;
		boss1CutsceneFlag1 = false;
		boss1CutsceneFinished = false;
		level2CutsceneStarted = false;
		level2CustsceneFinished = false;
		flashBangShow = false;
		flashBangHide = false;
		flashBangY1 = 360;
		flashBangY2 = 360;
		bulletPosition = Vector3f(0, 0, 0);
		boss1BulletPosition = Vector3f(0, 0, 0);
		boss2BulletPosition = Vector3f(0, 0, 0);
		bossBulletAngle = 0;
		playerIsHit = false;
		bossIsHit = false;
		objectPositions.clear();
		objectRanges.clear();
		loadStaticPositions(1);
		minutes = 0;
		seconds = 0;
		mciSendString(TEXT("stop dead"), NULL, 0, NULL);
		mciSendString(TEXT("close dead"), NULL, 0, NULL);
		mciSendString(TEXT("stop boss1shots"), NULL, 0, NULL);
		mciSendString(TEXT("close boss1shots"), NULL, 0, NULL);
		mciSendString(TEXT("stop boss2shots"), NULL, 0, NULL);
		mciSendString(TEXT("close boss2shots"), NULL, 0, NULL);
		mciSendString(TEXT("stop boss2dead"), NULL, 0, NULL);
		mciSendString(TEXT("close boss2dead"), NULL, 0, NULL);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		glDisable(GL_LIGHT3);
		glDisable(GL_LIGHT4);
		glDisable(GL_LIGHT5);
		glutTimerFunc(15000, Timer, 9);
		glutTimerFunc(0, Timer, 16);
		glutDisplayFunc(Display1);
		glutIdleFunc(Anim1);
	}
	// Level 2 started
	if (value == 1) {
		moveRate = 0.01;
		currentLevel = 2;
		camera.center = boss2ModelPosition;
		bulletPosition = Vector3f(0, 0, 0);
		playerIsHit = false;
		bossIsHit = false;
		objectPositions.clear();
		objectRanges.clear();
		loadStaticPositions(2);
		glEnable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		glDisable(GL_LIGHT3);
		glDisable(GL_LIGHT5);
		glutDisplayFunc(Display2);
		glutIdleFunc(Anim2);
	}
}

/* Scene Setup & Reshape */
void setupLights() {
	if(currentLevel == 1){
		// Thunder
		GLfloat light1_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat light1_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
		GLfloat light1_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat light1_position[] = { 0, 60, 0, 0.0f };
		GLfloat light1_constant_attenuation = 0.0f;
		GLfloat light1_linear_attenuation = 0.0f;
		GLfloat light1_quadratic_attenuation = 0.1f;
		glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
		glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
		glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
		glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, light1_constant_attenuation);
		glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, light1_linear_attenuation);
		glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, light1_quadratic_attenuation);

		// Boss 1 focus
		GLfloat light2_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat light2_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
		GLfloat light2_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat light2_position[] = { boss1ModelPosition.x, boss1ModelPosition.y, boss1ModelPosition.z, 1.0f };
		GLfloat light2_constant_attenuation = 0.0f;
		GLfloat light2_linear_attenuation = 0.0f;
		GLfloat light2_quadratic_attenuation = 0.1f;
		glLightfv(GL_LIGHT2, GL_AMBIENT, light2_ambient);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_diffuse);
		glLightfv(GL_LIGHT2, GL_SPECULAR, light2_specular);
		glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
		glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, light2_constant_attenuation);
		glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, light2_linear_attenuation);
		glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, light2_quadratic_attenuation);

		// Puzzle hall
		glEnable(GL_LIGHT3);
		GLfloat light3_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat light3_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
		GLfloat light3_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat light3_position[] = { 23.0f, 1.0f, 11.5f, 1.0f };
		GLfloat light3_constant_attenuation = 0.0f;
		GLfloat light3_linear_attenuation = 0.0f;
		GLfloat light3_quadratic_attenuation = 0.3f;
		glLightfv(GL_LIGHT3, GL_AMBIENT, light3_ambient);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, light3_diffuse);
		glLightfv(GL_LIGHT3, GL_SPECULAR, light3_specular);
		glLightfv(GL_LIGHT3, GL_POSITION, light3_position);
		glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, light3_constant_attenuation);
		glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, light3_linear_attenuation);
		glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, light3_quadratic_attenuation);

		// Traps
		GLfloat light5_ambient[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		GLfloat light5_diffuse[] = { 5.0f, 0.0f, 0.0f, 1.0f };
		GLfloat light5_specular[] = { 1.5f, 0.0f, 0.0f, 1.0f };
		GLfloat light5_position[] = { 30, 60, 0, 0.0f };
		GLfloat light5_constant_attenuation = 0.0f;
		GLfloat light5_linear_attenuation = 0.0f;
		GLfloat light5_quadratic_attenuation = 0.1f;
		glLightfv(GL_LIGHT5, GL_AMBIENT, light5_ambient);
		glLightfv(GL_LIGHT5, GL_DIFFUSE, light5_diffuse);
		glLightfv(GL_LIGHT5, GL_SPECULAR, light5_specular);
		glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION, light5_constant_attenuation);
		glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION, light5_linear_attenuation);
		glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, light5_quadratic_attenuation);
	}
	else {
		GLfloat light1_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		GLfloat light1_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
		GLfloat light1_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat light1_position[] = { boss2ModelPosition.x, boss2ModelPosition.y, boss2ModelPosition.z, 1.0f };
		GLfloat light1_constant_attenuation = 0.0f;
		GLfloat light1_linear_attenuation = 0.0f;
		GLfloat light1_quadratic_attenuation = 0.1f;
		glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
		glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
		glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
		glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, light1_constant_attenuation);
		glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, light1_linear_attenuation);
		glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, light1_quadratic_attenuation);
	}
	// Torch
	GLfloat light0_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat light0_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
	GLfloat light0_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat light0_position[] = { playerModelPosition.x, playerModelPosition.y, playerModelPosition.z, 1.0f };
	GLfloat light0_constant_attenuation = 0.0f;
	GLfloat light0_linear_attenuation = 0.0f;
	GLfloat light0_quadratic_attenuation = 0.4f;
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, light0_constant_attenuation);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, light0_linear_attenuation);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, light0_quadratic_attenuation);

	// weapon
	GLfloat light4_ambient[] = { 1.0f, 1.0f, 0.1f, 1.0f };
	GLfloat light4_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
	GLfloat light4_specular[] = { 1.0f, 1.0f, 0.5f, 1.0f };
	GLfloat light4_position[] = { playerModelPosition.x, playerModelPosition.y, playerModelPosition.z, 1.0f };
	GLfloat light4_constant_attenuation = 0.0f;
	GLfloat light4_linear_attenuation = 0.0f;
	GLfloat light4_quadratic_attenuation = 0.3f;
	glLightfv(GL_LIGHT4, GL_AMBIENT, light4_ambient);
	glLightfv(GL_LIGHT4, GL_DIFFUSE, light4_diffuse);
	glLightfv(GL_LIGHT4, GL_SPECULAR, light4_specular);
	glLightfv(GL_LIGHT4, GL_POSITION, light4_position);
	glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, light4_constant_attenuation);
	glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION, light4_linear_attenuation);
	glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, light4_quadratic_attenuation);


	/*
	glEnable(GL_LIGHT2);
	GLfloat light2_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat light2_diffuse[] = { 5.0f, 5.0f, 5.0f, 1.0f };
	GLfloat light2_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat light2_position[] = { playerModelPosition.x, playerModelPosition.y, playerModelPosition.z, 1.0f };
	GLfloat light2_constant_attenuation = 0.0f;
	GLfloat light2_linear_attenuation = 0.0f;
	GLfloat light2_quadratic_attenuation = 0.1f;
	//Vector3f lightDir = getVector(playerModelPosition, Vector3f(1, 1, 1));
	Vector3f lightDir = getViewVector();
	GLfloat light2_direction[] = { lightDir.x, lightDir.y, lightDir.z};
	glLightfv(GL_LIGHT2, GL_AMBIENT, light2_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_diffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light2_specular);
	glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light2_direction);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 15.0f);
	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 40.0);
	glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, light2_constant_attenuation);
	glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, light2_linear_attenuation);
	glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, light2_quadratic_attenuation);*/
}
void setupCamera(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, width / height, 0.01, 200);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.look();
}
void Reshape(int newWidth, int newHeight)
{
	if (newHeight == 0)
		newHeight = 1;
	width = newWidth;
	height = newHeight;
	glViewport(0, 0, newWidth, newHeight);
	setupCamera(newWidth, newHeight);
}
void loadModels() {
	// Models
	treeModel.Load("Models/tree/Tree1.3ds");
	mountainModel.Load("Models/mountain/mountain.3ds");
	wall0Model.Load("Models/walls/wall0.3ds");
	playerModel.Load("Models/player/player.3ds");
	gunModel.Load("Models/gun/gun.3DS");
	boss1Model.Load("Models/boss1/zombie.3ds");
	trap1Model.Load("Models/trap1/witch.3ds");
	treeLogModel.Load("Models/treeLog/treeLog.3ds");
	castleModel.Load("Models/castle/castle.3ds");
	dragonModel.Load("Models/boss2/dragon.3ds");

	// Textures
	glEnable(GL_TEXTURE_2D);
	groundTex.Load("Textures/ground.bmp");
	ground2Tex.Load("Textures/ground2.bmp");
	skyTex.Load("Textures/darkSky.bmp");
	sky2Tex.Load("Textures/orangeSky.bmp");
	gameoverTex.Load("Textures/gameover.bmp");
	moon1Tex.Load("Textures/moon1.bmp");
	moon2Tex.Load("Textures/moon2.bmp");
	trap2Tex.Load("Textures/trap2.bmp");
	trap3Tex.Load("Textures/trap3.bmp");
	fireTex.Load("Textures/fire.bmp");
	startTex.Load("Textures/start.bmp");
	controlsTex.Load("Textures/controls.bmp");
	aboutTex.Load("Textures/about.bmp");
	survivedTex.Load("Textures/survived.bmp");
}

/* Display Helpers */
void renderGround(GLTexture tex) {
	for (int i = 0;i <= 8;i++) {
		for (int j = 0;j <= 8;j++) {
			glPushMatrix();
			glBindTexture(GL_TEXTURE_2D, tex.texture[0]);
			glBegin(GL_QUADS);
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(0, 0);glVertex3f(0 + i * 10, 0, 0 + j * 10);
			glTexCoord2f(10, 0);glVertex3f(10 + i * 10, 0, 0 + j * 10);
			glTexCoord2f(10, 10);glVertex3f(10 + i * 10, 0, 10 + j * 10);
			glTexCoord2f(0, 10);glVertex3f(0 + i * 10, 0, 10 + j * 10);
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
			glPopMatrix();
		}
	}
}
void renderSky(GLTexture tex) {
	glPushMatrix();
	glDisable(GL_LIGHTING);
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);*/
	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glTranslated(0, 20, 0);
	glRotated(-150, 1, 0, 0);
	glBindTexture(GL_TEXTURE_2D, tex.texture[0]);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);
	gluSphere(qobj, 100, 40, 40);
	gluDeleteQuadric(qobj);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}
void renderMoon(GLTexture tex) {
	glPushMatrix();
	glDisable(GL_LIGHTING);
	/*glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);*/
	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glTranslated(0, 50, -60);
	glRotated(-70, 1, 0, 0);
	glBindTexture(GL_TEXTURE_2D, tex.texture[0]);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);
	gluSphere(qobj, moonRadius, 40, 40);
	gluDeleteQuadric(qobj);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}
void renderMaze() {
	// Maze
	// 1
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(2, 0, 4);
	glRotated(90, 0, 1, 0);
	glScaled(0.032, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 2 
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(2, 0, 10);
	glScaled(0.016, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 3
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(4, 0, 6);
	glRotated(90, 0, 1, 0);
	glScaled(0.032, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 4
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(7, 0, 5);
	glRotated(90, 0, 1, 0);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 5
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(5.5, 0, 8);
	glRotated(90, 0, 1, 0);
	glScaled(0.032, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 6
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(10, 0, 11.5);
	glScaled(0.08, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 7
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(16, 0, 10);
	glScaled(0.072, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 8
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(29, 0, 10);
	glScaled(0.032, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 9
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(30, 0, 11.5);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 10
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(29, 0, 13);
	glScaled(0.032, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 11
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(28.25, 0, 14.5);
	glScaled(0.026, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 12
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(35, 0, 11.5);
	glRotated(90, 0, 1, 0);
	glScaled(0.024, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 13
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(33, 0, 8.5);
	glRotated(90, 0, 1, 0);
	glScaled(0.012, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 14
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(58, 0, 7);
	glScaled(0.2, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 15
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(60, 0, 8.5);
	glScaled(0.2, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 16
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(33, 0, 14.5);
	glRotated(90, 0, 1, 0);
	glScaled(0.012, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 17
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(60, 0, 14.5);
	glScaled(0.2, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 18
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(58, 0, 16);
	glScaled(0.2, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 19
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(31.5, 0, 16);
	glRotated(90, 0, 1, 0);
	glScaled(0.012, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 20
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(56.5, 0, 17.5);
	glScaled(0.2, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 21
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(18, 0, 13);
	glScaled(0.016, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 22
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(20, 0, 13.75);
	glRotated(90, 0, 1, 0);
	glScaled(0.006, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 23
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(22.5, 0, 14.5);
	glScaled(0.02, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 24
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(16, 0, 13.75);
	glRotated(90, 0, 1, 0);
	glScaled(0.006, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 25
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(14.5, 0, 14.5);
	glRotated(90, 0, 1, 0);
	glScaled(0.024, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 26
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(22.25, 0, 17.5);
	glScaled(0.062, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 27
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(30, 0, 16.75);
	glRotated(90, 0, 1, 0);
	glScaled(0.006, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 28
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(5, 0, 30);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 29
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(5, 0, 31.5);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	if (boss1Health > 0) {
		// 30
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(10, 0, 30.75);
		glRotated(90, 0, 1, 0);
		glScaled(0.006, 0.02, 0.006);
		glRotated(-90, 0, 0, 1);
		wall0Model.Draw();
		glPopMatrix();
	}
	if (boss1FightStart) {
		// 31
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(30.75, 0, 17.5);
		glScaled(0.006, 0.02, 0.006);
		glRotated(-90, 0, 0, 1);
		wall0Model.Draw();
		glPopMatrix();
	}
}
void renderLevel1Design() {
// Game Boundries //
	// xAxis
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(10, 0, 0);
	glScaled(0.08, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// zAxis
	// Part 1
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(0, 0, 15);
	glRotated(90, 0, 1, 0);
	glScaled(0.12, 0.05, 0.015);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// Part 2
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(0, 0, 55.75);
	glRotated(90, 0, 1, 0);
	glScaled(0.194, 0.05, 0.015);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
// Scene Design //
	// Mountains
	// +ve X
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(80, 0, 40);
	glScaled(5, 10, 10);
	glRotated(180, 0, 1, 0);
	mountainModel.Draw();
	glPopMatrix();
	// +ve Z
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(40, 0, 80);
	glScaled(15, 10, 5);
	mountainModel.Draw();
	glPopMatrix();

	// Maze
	renderMaze();
}
void print(int x, int y, char* string)
{
	int len, i;
	glRasterPos2f(x, y);
	len = (int)strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
	}
}

/* Collision Detection */
void loadStaticPositions(int value) {
	objectPositions["xAxis"] = Vector3f(0, 0, 0);
	objectRanges["xAxis"] = Vector3f(80, 1, 0.25);

	objectPositions["zAxis"] = Vector3f(0, 0, 0);
	objectRanges["zAxis"] = Vector3f(0.25, 1, 80);

	if (value == 1) {
		objectPositions["mountainX+"] = Vector3f(80, 0, 40);
		objectRanges["mountainX+"] = Vector3f(15, 30, 40);

		objectPositions["mountainZ+"] = Vector3f(40, 0, 80);
		objectRanges["mountainZ+"] = Vector3f(40, 15, 20);
		if (triggered) {
			objectPositions["trap1"] = Vector3f(35, 0, 7.75);
			objectRanges["trap1"] = Vector3f(0.05, 1, 0.75);
		
			objectPositions["trap2"] = Vector3f(35, 0, 15.25);
			objectRanges["trap2"] = Vector3f(0.1, 1, 0.75);

			objectPositions["trap3"] = Vector3f(33, 0, 16.75);
			objectRanges["trap3"] = Vector3f(0.1, 1, 0.75);
		}

		objectPositions["maze1"] = Vector3f(2, 0, 4);
		objectRanges["maze1"] = Vector3f(0.05, 1, 4);
		objectPositions["maze2"] = Vector3f(2, 0, 10);
		objectRanges["maze2"] = Vector3f(2, 1, 0.05);
		objectPositions["maze3"] = Vector3f(4, 0, 6);
		objectRanges["maze3"] = Vector3f(0.05, 1, 4);
		objectPositions["maze4"] = Vector3f(7, 0, 5);
		objectRanges["maze4"] = Vector3f(0.05, 1, 5);
		objectPositions["maze5"] = Vector3f(5.5, 0, 8);
		objectRanges["maze5"] = Vector3f(0.05, 1, 4);
		objectPositions["maze6"] = Vector3f(10, 0, 11.5);
		objectRanges["maze6"] = Vector3f(10, 1, 0.1);
		objectPositions["maze7"] = Vector3f(16, 0, 10);
		objectRanges["maze7"] = Vector3f(9, 1, 0.05);
		objectPositions["maze8"] = Vector3f(29, 0, 10);
		objectRanges["maze8"] = Vector3f(4, 1, 0.05);
		objectPositions["maze9"] = Vector3f(30, 0, 11.5);
		objectRanges["maze9"] = Vector3f(5, 1, 0.05);
		objectPositions["maze10"] = Vector3f(29, 0, 13);
		objectRanges["maze10"] = Vector3f(4, 1, 0.05);
		objectPositions["maze11"] = Vector3f(28.25, 0, 14.5);
		objectRanges["maze11"] = Vector3f(3.25, 1, 0.05);
		objectPositions["maze12"] = Vector3f(35, 0, 11.5);
		objectRanges["maze12"] = Vector3f(0.05, 1, 3);
		objectPositions["maze13"] = Vector3f(33, 0, 8.5);
		objectRanges["maze13"] = Vector3f(0.05, 1, 1.5);
		objectPositions["maze14"] = Vector3f(50, 0, 7);
		objectRanges["maze14"] = Vector3f(25, 1, 0.05);
		objectPositions["maze15"] = Vector3f(60, 0, 8.5);
		objectRanges["maze15"] = Vector3f(25, 1, 0.05);
		objectPositions["maze16"] = Vector3f(33, 0, 14.5);
		objectRanges["maze16"] = Vector3f(0.05, 1, 1.5);
		objectPositions["maze17"] = Vector3f(60, 0, 14.5);
		objectRanges["maze17"] = Vector3f(25, 1, 0.05);
		objectPositions["maze18"] = Vector3f(58, 0, 16);
		objectRanges["maze18"] = Vector3f(25, 1, 0.05);
		objectPositions["maze19"] = Vector3f(31.5, 0, 16);
		objectRanges["maze19"] = Vector3f(0.05, 1, 1.5);
		objectPositions["maze20"] = Vector3f(56.5, 0, 17.5);
		objectRanges["maze20"] = Vector3f(25, 1, 0.05);
		objectPositions["maze21"] = Vector3f(18, 0, 13);
		objectRanges["maze21"] = Vector3f(2, 1, 0.05);
		objectPositions["maze22"] = Vector3f(20, 0, 13.75);
		objectRanges["maze22"] = Vector3f(0.05, 1, 0.75);
		objectPositions["maze23"] = Vector3f(22.5, 0, 14.5);
		objectRanges["maze23"] = Vector3f(2.5, 1, 0.05);
		objectPositions["maze24"] = Vector3f(16, 0, 13.75);
		objectRanges["maze24"] = Vector3f(0.05, 1, 0.75);
		objectPositions["maze25"] = Vector3f(14.5, 0, 14.5);
		objectRanges["maze25"] = Vector3f(0.05, 1, 3);
		objectPositions["maze26"] = Vector3f(22.25, 0, 17.5);
		objectRanges["maze26"] = Vector3f(7.75, 1, 0.05);
		objectPositions["maze27"] = Vector3f(30, 0, 16.75);
		objectRanges["maze27"] = Vector3f(0.05, 1, 0.75);
		objectPositions["maze28"] = Vector3f(5, 0, 30);
		objectRanges["maze28"] = Vector3f(5, 1, 0.05);
		objectPositions["maze29"] = Vector3f(5, 0, 31.5);
		objectRanges["maze29"] = Vector3f(5, 1, 0.05);
		if (boss1Health > 0) {
			objectPositions["maze30"] = Vector3f(10, 0, 30.75);
			objectRanges["maze30"] = Vector3f(0.05, 1, 0.75);
		}
		if (boss1FightStart) {
			objectPositions["maze31"] = Vector3f(30.75, 0, 17.5);
			objectRanges["maze31"] = Vector3f(0.75, 1, 0.05);
		}
		objectPositions["boss1Model"] = boss1ModelPosition;
		objectRanges["boss1Range"] = boss1ModelRange;
	}
	if (value == 2) {
		objectPositions["castleX+"] = Vector3f(80, 0, 40);
		objectRanges["castleX+"] = Vector3f(15, 30, 40);

		objectPositions["mountainZ+"] = Vector3f(40, 0, 80);
		objectRanges["mountainZ+"] = Vector3f(40, 15, 20);

		objectPositions["mountain2"] = Vector3f(40, 0, 0);
		objectRanges["mountain2"] = Vector3f(40, 15, 20);

		objectPositions["mountain3"] = Vector3f(-20, 0, 40);
		objectRanges["mountain3"] = Vector3f(20, 15, 40);

		objectPositions["maze28"] = Vector3f(5, 0, 30);
		objectRanges["maze28"] = Vector3f(5, 1, 0.05);
		objectPositions["maze29"] = Vector3f(5, 0, 31.5);
		objectRanges["maze29"] = Vector3f(5, 1, 0.05);

		objectPositions["boss2Model"] = boss2ModelPosition;
		objectRanges["boss2Range"] = boss2ModelRange;
	}
}
bool boundingBoxIntersection(Vector3f posA, Vector3f rangeA, Vector3f posB, Vector3f rangeB) {
	float aMinX = posA.x - rangeA.x;
	float aMaxX = posA.x + rangeA.x;
	float aMinY = posA.y - rangeA.y;
	float aMaxY = posA.y + rangeA.y;
	float aMinZ = posA.z - rangeA.z;
	float aMaxZ = posA.z + rangeA.z;

	float bMinX = posB.x - rangeB.x;
	float bMaxX = posB.x + rangeB.x;
	float bMinY = posB.y - rangeB.y;
	float bMaxY = posB.y + rangeB.y;
	float bMinZ = posB.z - rangeB.z;
	float bMaxZ = posB.z + rangeB.z;

	return (aMinX <= bMaxX && aMaxX >= bMinX) &&
		(aMinY <= bMaxY && aMaxY >= bMinY) &&
		(aMinZ <= bMaxZ && aMaxZ >= bMinZ);
}
bool checkCollision(Vector3f pos, Vector3f range) {
	for (auto const& x : objectPositions)
	{
		if (boundingBoxIntersection(pos, range, x.second, objectRanges[x.first])) {
			return true;
		}
	}
	return false;
}

/* Shooting Helpers */
Vector3f getViewVector() {
	Vector3f myView = (camera.center - camera.eye).unit();
	return myView;
}
Vector3f getRightVector() {
	Vector3f myRight = camera.up.cross(camera.center - camera.eye).unit();
	return myRight;
}
Vector3f getUpVector() {
	Vector3f myUp = camera.up.unit();
	return myUp;
}
Vector3f getVector(Vector3f a, Vector3f b) {
	Vector3f target;
	target.x = b.x - a.x;
	target.y = b.y - a.y;
	target.z = b.z - a.z;
	return target.unit();
}
void fixToCameraCenter() {
	Vector3f view = getViewVector();
	bulletPosition = camera.eye + view * 0.5;
}
void fixGun() {
	Vector3f view = getViewVector();
	gunModelPosition = camera.eye + view * 0.5;
}

/* Player Model Helpers */
void rotatePlayer(Vector3f direction) {
	float angle = atan2f(direction.x, direction.z) * 180.0 / PI;
	playerRotation = angle;

	Vector3f view = getViewVector();
	view.y = 0;
	Vector3f right = getRightVector();
	right.y = 0;
	gunModelPosition = playerModelPosition + view * 0.1 + (cameraMode ? right * -0.05 : 0);
	gunModelPosition.y = playerModelPosition.y + cameraMode? 0.5 : 0.3;
}
void rotateWeaponX(Vector3f direction) {
	float angle = atan2f(direction.y, direction.z) * 180.0 / PI;
	weaponRotationX = angle;
}
void movePlayerX(float d) {
	Vector3f right = getRightVector();
	right.y = 0;
	playerModelPosition = playerModelPosition + right * d;

	Vector3f view = getViewVector();
	view.y = 0;
	gunModelPosition = playerModelPosition + view * 0.1 + (cameraMode ? right * -0.05 : 0);
	gunModelPosition.y = playerModelPosition.y + cameraMode ? 0.5 : 0.3;
}
void movePlayerY(float d) {
	playerModelPosition.y = playerModelPosition.y + d;

	Vector3f view = getViewVector();
	view.y = 0;
	Vector3f right = getRightVector();
	right.y = 0;
	gunModelPosition = playerModelPosition + view * 0.1 + (cameraMode ? right * -0.05 : 0);
	gunModelPosition.y = playerModelPosition.y + cameraMode ? 0.5 : 0.3;
}
void movePlayerZ(float d) {
	Vector3f view = getViewVector();
	view.y = 0;
	playerModelPosition = playerModelPosition + view * d;

	Vector3f right = getRightVector();
	right.y = 0;
	gunModelPosition = playerModelPosition + view * 0.1 + (cameraMode? right * -0.05 : 0);
	gunModelPosition.y = playerModelPosition.y + cameraMode ? 0.5 : 0.3;
}

/* Boss Model Helpers */
void rotateBoss(Vector3f direction, int bossID) {
	//float dotProduct = newDir.dot(oldDir);
	//float oldDirMag = oldDir.magnitude();
	//float newDirMag = newDir.magnitude();
	//float angle = acos(dotProduct) * 180.0 / PI;
	float angle = atan2f(direction.x, direction.z) * 180.0 / PI;
	if (bossID == 1) {
		boss1Rotation = angle;
	}
	else {
		boss2Rotation = angle;
	}
}
void moveBossX(float d, int bossID) {
	// boss 1
	if (bossID == 1) {
		Vector3f right = camera.up.cross(playerModelPosition - boss1ModelPosition).unit();
		right.y = 0;
		boss1ModelPosition = boss1ModelPosition + right * d;

		Vector3f viewNN = (playerModelPosition - boss1ModelPosition);
		viewNN.y = 0;
		rotateBoss(viewNN, bossID);
	}
	// boss 2
	else {
		Vector3f right = camera.up.cross(playerModelPosition - boss2ModelPosition).unit();
		right.y = 0;
		boss2ModelPosition = boss2ModelPosition + right * d;

		Vector3f viewNN = (playerModelPosition - boss2ModelPosition);
		viewNN.y = 0;
		rotateBoss(viewNN, bossID);
	}
}
void moveBossY(float d) {
	boss2ModelPosition.y = boss2ModelPosition.y + d;
}
void moveBossZ(float d, int bossID) {
	// boss 1
	if (bossID == 1) {
		Vector3f view = (playerModelPosition - boss1ModelPosition).unit();
		view.y = 0;
		boss1ModelPosition = boss1ModelPosition + view * d;

		Vector3f viewNN = (playerModelPosition - boss1ModelPosition);
		viewNN.y = 0;
		rotateBoss(viewNN, bossID);
	}
	// boss 2
	else {
		Vector3f view = (playerModelPosition - boss2ModelPosition).unit();
		view.y = 0;
		boss2ModelPosition = boss2ModelPosition + view * d;

		Vector3f viewNN = (playerModelPosition - boss2ModelPosition);
		viewNN.y = 0;
		rotateBoss(viewNN, bossID);
	}

}

/* Controls */
void Keyboard(unsigned char key, int x, int y) {
	if (key == GLUT_KEY_ESCAPE)
		exit(EXIT_SUCCESS);
	if (!boss1Cutscene && playerHealth > 0 && boss2Health > 0 && currentLevel > 0 && ((!level2CutsceneStarted && !level2CustsceneFinished) || (level2CutsceneStarted && level2CustsceneFinished))) {
		keyboardBuffer[key] = true;
		if (key == 'v') {
			camera.switchMode(cameraMode, playerModelPosition);
			cameraMode = !cameraMode;

			Vector3f view = getViewVector();
			view.y = 0;
			Vector3f right = getRightVector();
			right.y = 0;
			gunModelPosition = playerModelPosition + view * 0.1 + (cameraMode ? right * -0.05 : 0);
			gunModelPosition.y = playerModelPosition.y + cameraMode ? 0.5 : 0.3;
		}
		if (key == 'f' && !triggered) {
			flashOn ? glDisable(GL_LIGHT0) : glEnable(GL_LIGHT0);
			flashOn = !flashOn;
			mciSendString(TEXT("stop switch"), NULL, 0, NULL);
			mciSendString(TEXT("close switch"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/switch.wav\" alias switch"), NULL, 0, NULL);
			mciSendString(TEXT("play switch"), NULL, 0, NULL);
		}
		if ((key == 'w' || key == 'a' || key == 's' || key == 'd') && !walking) {
			printf("%c \n", key);
			mciSendString(TEXT("stop footsteps"), NULL, 0, NULL);
			mciSendString(TEXT("close footsteps"), NULL, 0, NULL);
			walking = true;
			glutTimerFunc(0, Timer, 15);
		}
		if (debugMode) {
			if (key == '=') {
				moveRate += 0.01;
				printf("%f \n", moveRate);
			}
			if (key == '-') {
				if (moveRate > 0.01)
					moveRate -= 0.01;
				else {
					moveRate -= 0.001;
				}
				printf("%f \n", moveRate);
			}
			//printf("%f %f %f \n", playerModelPosition.x, playerModelPosition.y, playerModelPosition.z);
		}
	}
	glutPostRedisplay();
}
void KeyboardUp(unsigned char key, int x, int y) {
	keyboardBuffer[key] = false;
	if ((key == 'w' || key == 'a' || key == 's' || key == 'd') && walking && keyboardBuffer['w'] == false && keyboardBuffer['a'] == false && keyboardBuffer['s'] == false && keyboardBuffer['d'] == false) {
		walking = false;
		mciSendString(TEXT("stop footsteps"), NULL, 0, NULL);
		mciSendString(TEXT("close footsteps"), NULL, 0, NULL);
	}
	glutPostRedisplay();
}
void Special(int key, int x, int y) {
	if (debugMode) {
		switch (key) {
		case GLUT_KEY_UP:
			camera.rotateX(lookRate, cameraMode);
			break;
		case GLUT_KEY_DOWN:
			camera.rotateX(-lookRate, cameraMode);
			break;
		case GLUT_KEY_LEFT:
			camera.rotateZ(lookRate, cameraMode);
			break;
		case GLUT_KEY_RIGHT:
			camera.rotateZ(-lookRate, cameraMode);
			break;
		}
	}
	glutPostRedisplay();
}
void SpecialUp(int key, int x, int y) {
	//glutPostRedisplay();
}
void Mouse(int button, int state, int x, int y) {
	if (!boss1Cutscene && playerHealth > 0 && boss2Health > 0 && currentLevel > 0 && ((!level2CutsceneStarted && !level2CustsceneFinished) || (level2CutsceneStarted && level2CustsceneFinished))) {
		y = 720 - y;
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			if (!triggered) {
				glEnable(GL_LIGHT4);
			}
			bossIsHit = false;
			fixToCameraCenter();
			bulletDirection = getViewVector();
			mciSendString(TEXT("stop gunfire"), NULL, 0, NULL);
			mciSendString(TEXT("close gunfire"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/gunfire.wav\" alias gunfire"), NULL, 0, NULL);
			mciSendString(TEXT("play gunfire"), NULL, 0, NULL);
			glutTimerFunc(100, Timer, 11);
		}
	}
	if (playerHealth == 0 || boss2Health == 0) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			// Play again
			if (x > 150 && x < 560 && y>500 && y < 560) {
				mciSendString(TEXT("stop select"), NULL, 0, NULL);
				mciSendString(TEXT("close select"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/select.wav\" alias select"), NULL, 0, NULL);
				mciSendString(TEXT("play select"), NULL, 0, NULL);
				reset(0);
				glutSetCursor(GLUT_CURSOR_NONE);
			}
			// Quit
			if (x > 830 && x < 970 && y>500 && y < 560) {
				exit(EXIT_SUCCESS);
			}
		}
	}
	if (currentLevel == 0) {
		if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
			// Play Start
			if (x > 128 && x < 306 && y>494 && y < 537 && start) {
				mciSendString(TEXT("stop introMusic"), NULL, 0, NULL);
				mciSendString(TEXT("close introMusic"), NULL, 0, NULL);
				mciSendString(TEXT("stop select"), NULL, 0, NULL);
				mciSendString(TEXT("close select"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/select.wav\" alias select"), NULL, 0, NULL);
				mciSendString(TEXT("play select"), NULL, 0, NULL);
				reset(0);
				glutDisplayFunc(Display1);
				glutIdleFunc(Anim1);
				glutSetCursor(GLUT_CURSOR_NONE);
			}
			// controls
			if (x > 500 && x < 830 && y>500 && y < 537 && start) {
				mciSendString(TEXT("stop select"), NULL, 0, NULL);
				mciSendString(TEXT("close select"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/select.wav\" alias select"), NULL, 0, NULL);
				mciSendString(TEXT("play select"), NULL, 0, NULL);
				start = false;
				controls = true;
			}
			// about
			if (x > 1035 && x < 1225 && y>500 && y < 537 && start) {
				mciSendString(TEXT("stop select"), NULL, 0, NULL);
				mciSendString(TEXT("close select"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/select.wav\" alias select"), NULL, 0, NULL);
				mciSendString(TEXT("play select"), NULL, 0, NULL);
				start = false;
				about = true;
			}
			// back
			if (x > 603 && x < 730 && y>650 && y < 690 && !start) {
				mciSendString(TEXT("stop select"), NULL, 0, NULL);
				mciSendString(TEXT("close select"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/select.wav\" alias select"), NULL, 0, NULL);
				mciSendString(TEXT("play select"), NULL, 0, NULL);
				start = true;
				controls = false;
				about = false;
			}
		}
	}
	glutPostRedisplay();
}
void MouseMotion(int x, int y) {
	y = 720 - y;
	if (!boss1Cutscene && playerHealth > 0 && boss2Health > 0 && currentLevel>0 && ((!level2CutsceneStarted && !level2CustsceneFinished) || (level2CutsceneStarted && level2CustsceneFinished))) {
		int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
		int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
		if (x != centerX || y != centerY)
			glutWarpPointer(centerX, centerY);
		float xOffset = x - centerX;
		float yOffset = y - centerY;
		xOffset *= sensitivity;
		yOffset *= sensitivity;
		if (rotationX + yOffset < 40 && rotationX + yOffset > -40) {
			rotationX += yOffset;
			camera.rotateX(yOffset, cameraMode);
		}
		rotationY += -xOffset;
		camera.rotateY(-xOffset, cameraMode);

		Vector3f viewNN = (camera.center - camera.eye);
		viewNN.y = 0;
		rotatePlayer(viewNN);
		Vector3f tempView = (camera.center - camera.eye).unit();
		Vector3f newCenter = camera.center + tempView * 2;
		viewNN = (newCenter - gunModelPosition);
		viewNN.x = 0;
		rotateWeaponX(viewNN);
	}
	glutPostRedisplay();
}
void MouseClickMotion(int x, int y) {
	y = 720 - y;
	if (!boss1Cutscene && playerHealth > 0 && boss2Health > 0 && currentLevel > 0 && ((!level2CutsceneStarted && !level2CustsceneFinished) || (level2CutsceneStarted && level2CustsceneFinished))) {
		int centerX = glutGet(GLUT_WINDOW_WIDTH) / 2;
		int centerY = glutGet(GLUT_WINDOW_HEIGHT) / 2;
		if (x != centerX || y != centerY)
			glutWarpPointer(centerX, centerY);
		float xOffset = x - centerX;
		float yOffset = y - centerY;
		xOffset *= sensitivity;
		yOffset *= sensitivity;
		if (rotationX + yOffset < 40 && rotationX + yOffset > -40) {
			rotationX += yOffset;
			camera.rotateX(yOffset, cameraMode);
		}
		rotationY += -xOffset;
		camera.rotateY(-xOffset, cameraMode);

		Vector3f viewNN = (camera.center - camera.eye);
		viewNN.y = 0;
		rotatePlayer(viewNN);
		Vector3f tempView = (camera.center - camera.eye).unit();
		Vector3f newCenter = camera.center + tempView * 2;
		viewNN = (newCenter - gunModelPosition);
		viewNN.x = 0;
		rotateWeaponX(viewNN);
	}
	glutPostRedisplay();
}

/* Timers */
void Timer(int value) {
	// Generate boss 1 random movement
	if (value == 1) {
		bossMoveX = rand() % 2;
		bossMoveZ = rand() % 2;
		if (boss1Health > 0) {
			glutTimerFunc(2000, Timer, 1);
		}
	}
	// Generate boss 1 shoots
	if (value == 2 && boss1Health > 0) {
		mciSendString(TEXT("stop boss1shots"), NULL, 0, NULL);
		mciSendString(TEXT("close boss1shots"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/boss1shots.wav\" alias boss1shots"), NULL, 0, NULL);
		mciSendString(TEXT("play boss1shots"), NULL, 0, NULL);
		playerIsHit = false;
		Vector3f playerModelPositionCentered = playerModelPosition;
		playerModelPositionCentered.y = playerModelPositionCentered.y + 0.45;
		boss1BulletDirection = getVector(boss1ModelPosition, playerModelPositionCentered);
		boss1BulletPosition = boss1ModelPosition;
		if (boss1Health > 0) {
			glutTimerFunc((rand() % 3000)+4000, Timer, 2);
		}
	}
	// Trigger boss 1 cutscene flag 1
	if (value == 3) {
		mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/cutscene_1_sound00.wav\" alias cutscene_1_sound00"), NULL, 0, NULL);
		mciSendString(TEXT("play cutscene_1_sound00"), NULL, 0, NULL);
		glEnable(GL_LIGHT2);
		glutTimerFunc(8000, Timer, 4);
	}
	if (value == 4) {
		mciSendString(TEXT("stop cutscene_1_sound00"), NULL, 0, NULL);
		mciSendString(TEXT("close cutscene_1_sound00"), NULL, 0, NULL);
		boss1CutsceneFinished = true;
	}
	// Generate boss 2 random movement
	if (value == 5) {
		bossMoveX = rand() % 2;
		bossMoveY = rand() % 2;
		bossMoveZ = rand() % 2;
		if (boss2Health > 0) {
			glutTimerFunc(5000 + (rand() % 2000), Timer, 5);
		}
	}
	// Generate boss 2 shoots
	if (value == 6 && boss2Health > 0 && currentLevel == 2 && playerHealth>0) {
		mciSendString(TEXT("stop boss2shots"), NULL, 0, NULL);
		mciSendString(TEXT("close boss2shots"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/boss2shots.wav\" alias boss2shots"), NULL, 0, NULL);
		mciSendString(TEXT("play boss2shots"), NULL, 0, NULL);
		playerIsHit = false;
		Vector3f playerModelPositionCentered = playerModelPosition;
		playerModelPositionCentered.y = playerModelPositionCentered.y + 0.45;
		boss2BulletDirection = getVector(boss2ModelPosition, playerModelPositionCentered);
		boss2BulletPosition = boss2ModelPosition;
		if (boss2Health > 0) {
			glutTimerFunc(6000 + (rand()%4000), Timer, 6);
		}
	}
	// Level 2 Cutscene flashbang show
	if (value == 7) {
		flashBangShow = true;
		glutTimerFunc(14500, Timer, 8);
	}
	// Level 2 Cutscene flashbang hide ( now i am in level 2)
	if (value == 8) {
		flashBangHide = true;
		level2CustsceneFinished = true;
		mciSendString(TEXT("stop cutscene_2_sound00"), NULL, 0, NULL);
		mciSendString(TEXT("close cutscene_2_sound00"), NULL, 0, NULL);
		camera.eye.y = camera.eye.y - 1;
		reset(1);
	}
	// thunder
	if (value == 9 && currentLevel == 1) {
		if (!boss1Cutscene && playerHealth > 0 && !triggered) {
			mciSendString(TEXT("stop thunder"), NULL, 0, NULL);
			mciSendString(TEXT("close thunder"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/thunder.wav\" alias thunder"), NULL, 0, NULL);
			mciSendString(TEXT("play thunder"), NULL, 0, NULL);
			thunderCount = (rand() % 10) + 1;
			glutTimerFunc(0, Timer, 10);
			glutTimerFunc(6000 + (rand() % 10000), Timer, 9);
		}
	}
	if (value == 10) {
		if (!boss1Cutscene && playerHealth > 0 && !triggered) {
			thunderCount--;
			if (thunderCount > 0) {
				thunderOn ? glDisable(GL_LIGHT1) : glEnable(GL_LIGHT1);
				thunderOn = !thunderOn;
				glutTimerFunc(100, Timer, 10);
			}
			else {
				glDisable(GL_LIGHT1);
			}
		}
		else {
			glDisable(GL_LIGHT1);
		}
	}
	// Gun
	if (value == 11) {
		glDisable(GL_LIGHT4);
	}
	// trap 1
	if (value == 12) {
		trap1 = true;
		mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/trap1.wav\" alias trap1"), NULL, 0, NULL);
		mciSendString(TEXT("play trap1"), NULL, 0, NULL);
	}
	// trap 2
	if (value == 13) {
		trap2 = true;
		mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/trap2.wav\" alias trap2"), NULL, 0, NULL);
		mciSendString(TEXT("play trap2"), NULL, 0, NULL);
	}
	// trap 3
	if (value == 14) {
		trap3 = true;
		mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/trap3.wav\" alias trap3"), NULL, 0, NULL);
		mciSendString(TEXT("play trap3"), NULL, 0, NULL);
	}
	// walking
	if (value == 15 && walking) {
		mciSendString(TEXT("stop footsteps"), NULL, 0, NULL);
		mciSendString(TEXT("close footsteps"), NULL, 0, NULL);
		mciSendString(TEXT("open \"Sounds/footsteps.wav\" alias footsteps"), NULL, 0, NULL);
		mciSendString(TEXT("play footsteps"), NULL, 0, NULL);
		/*if (walking) {
			glutTimerFunc(21500, Timer, 15);
		}*/
	}
	// Time
	if (value == 16) {
		if (seconds + 1 == 60) {
			seconds = 0;
			minutes++;
		}
		else {
			seconds++;
		}
		if (boss2Health > 0 && playerHealth >0) {
			glutTimerFunc(1000, Timer, 16);
		}
	}
}

/* Animation */
void Anim1()
{
	if (!boss1Cutscene && playerHealth > 0 && ((!level2CutsceneStarted && !level2CustsceneFinished) || (level2CutsceneStarted && level2CustsceneFinished))) {
		// Controls
		if (keyboardBuffer['q']) {
			Vector3f newPos = playerModelPosition;
			newPos.y = newPos.y + moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveY(moveRate);
				movePlayerY(moveRate);
			}
		}
		if (keyboardBuffer['e']) {
			Vector3f newPos = playerModelPosition;
			newPos.y = newPos.y - moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveY(-moveRate);
				movePlayerY(-moveRate);
			}
		}
		if (keyboardBuffer['a']) {
			Vector3f newPos = playerModelPosition;
			Vector3f right = getRightVector();
			right.y = 0;
			newPos = newPos + right * moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveX(moveRate);
				movePlayerX(moveRate);
			}
		}
		if (keyboardBuffer['d']) {
			Vector3f newPos = playerModelPosition;
			Vector3f right = getRightVector();
			right.y = 0;
			newPos = newPos + right * -moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveX(-moveRate);
				movePlayerX(-moveRate);
			}
		}
		if (keyboardBuffer['w']) {
			Vector3f newPos = playerModelPosition;
			Vector3f view = getViewVector();
			view.y = 0;
			newPos = newPos + view * moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveZ(moveRate);
				movePlayerZ(moveRate);
			}
		}
		if (keyboardBuffer['s']) {
			Vector3f newPos = playerModelPosition;
			Vector3f view = getViewVector();
			view.y = 0;
			newPos = newPos + view * -moveRate;
			if (!checkCollision(newPos, playerModelRange)) {
				camera.moveZ(-moveRate);
				movePlayerZ(-moveRate);
			}
		}
		// Player Bullets
		bulletPosition = bulletPosition + bulletDirection.unit() * playerBulletSpeed;
		if (!bossIsHit && boss1Health > 0 && playerHealth > 0) {
			bool bossHit = boundingBoxIntersection(boss1ModelPosition, boss1ModelRange, bulletPosition, bulletRange);
			if (bossHit && boss1FightStart) {
				bossIsHit = true;
				boss1Health--;
				boss1HealthX -= boss1HealthDownRate;
				if (boss1Health == 0) {
					mciSendString(TEXT("stop boss1shots"), NULL, 0, NULL);
					mciSendString(TEXT("close boss1shots"), NULL, 0, NULL);
					mciSendString(TEXT("stop boss1dead"), NULL, 0, NULL);
					mciSendString(TEXT("close boss1dead"), NULL, 0, NULL);
					mciSendString(TEXT("open \"Sounds/boss1dead.wav\" alias boss1dead"), NULL, 0, NULL);
					mciSendString(TEXT("play boss1dead"), NULL, 0, NULL);
					glDisable(GL_LIGHT2);
					objectPositions.clear();
					objectRanges.clear();
					loadStaticPositions(1);
				}
				else {
					mciSendString(TEXT("stop bossDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("close bossDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("open \"Sounds/bossDamaged.wav\" alias bossDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("play bossDamaged"), NULL, 0, NULL);
				}
			}
		}
		// Boss 1 Movements & Shootings
		if (boss1Health > 0 && boss1FightStart && playerHealth > 0) {
			if (bossMoveX == 0) {
				if (boss1ModelPosition.x - boss1MoveRate > 20) {
					moveBossX(-boss1MoveRate, 1);
				}
			}
			else {
				if (boss1ModelPosition.x + boss1MoveRate < 60) {
					moveBossX(boss1MoveRate, 1);
				}
			}
			if (bossMoveZ == 0) {
				if (boss1ModelPosition.z - boss1MoveRate > 20) {
					moveBossZ(-boss1MoveRate, 1);
				}
			}
			else {
				if (boss1ModelPosition.x + boss1MoveRate < 60) {
					moveBossZ(boss1MoveRate, 1);
				}
			}
			boss1BulletPosition = boss1BulletPosition + boss1BulletDirection * bossBulletSpeed;
			if (!playerIsHit) {
				bool bossHitPlayer = boundingBoxIntersection(playerModelPosition, playerModelRange, boss1BulletPosition, boss1BulletRange);
				if (bossHitPlayer) {
					playerIsHit = true;
					playerHealth-=1;
					playerHealthY += playerHealthDownRate;
					printf("%f \n", playerHealth);
					if (playerHealth <= 0) {
						mciSendString(TEXT("stop dead"), NULL, 0, NULL);
						mciSendString(TEXT("close dead"), NULL, 0, NULL);
						mciSendString(TEXT("open \"Sounds/dead.wav\" alias dead"), NULL, 0, NULL);
						mciSendString(TEXT("play dead"), NULL, 0, NULL);
						glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
					}
					else {
						mciSendString(TEXT("stop playerDamaged"), NULL, 0, NULL);
						mciSendString(TEXT("close playerDamaged"), NULL, 0, NULL);
						mciSendString(TEXT("open \"Sounds/playerDamaged.wav\" alias playerDamaged"), NULL, 0, NULL);
						mciSendString(TEXT("play playerDamaged"), NULL, 0, NULL);
					}
				}
			}
		}
		// Check if trap 1 triggered
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(40, 0, 7.75), Vector3f(0.05, 1, 0.75)) && !triggered) {
			triggered = true;
			mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/trapTrigger.wav\" alias trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("play trapTrigger"), NULL, 0, NULL);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
			glDisable(GL_LIGHT3);
			glDisable(GL_LIGHT4);
			glEnable(GL_LIGHT5);
			loadStaticPositions(1);
			glutTimerFunc(20500, Timer, 12);
		}
		if (trap1) {
			trap1Direction = getVector(trap1Position, playerModelPosition);
			trap1Position = trap1Position + trap1Direction * trap1MoveRate;
			if (boundingBoxIntersection(playerModelPosition, playerModelRange, trap1Position, trap1Range)) {
				mciSendString(TEXT("stop trap1"), NULL, 0, NULL);
				mciSendString(TEXT("close trap1"), NULL, 0, NULL);
				mciSendString(TEXT("stop dead"), NULL, 0, NULL);
				mciSendString(TEXT("close dead"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/dead.wav\" alias dead"), NULL, 0, NULL);
				mciSendString(TEXT("play dead"), NULL, 0, NULL);
				playerHealth = 0;
				glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
			}
		}
		// Check if trap 2 triggered
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(40, 0, 15.25), Vector3f(0.05, 1, 0.75)) && !triggered) {
			triggered = true;
			mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/trapTrigger.wav\" alias trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("play trapTrigger"), NULL, 0, NULL);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
			glDisable(GL_LIGHT3);
			glDisable(GL_LIGHT4);
			glEnable(GL_LIGHT5);
			loadStaticPositions(1);
			glutTimerFunc(20500, Timer, 13);
		}
		if (trap2) {
			trap2Position.x = trap2Position.x - trap2MoveRate;
			if (boundingBoxIntersection(playerModelPosition, playerModelRange, trap2Position, trap2Range)) {
				mciSendString(TEXT("stop trap2"), NULL, 0, NULL);
				mciSendString(TEXT("close trap2"), NULL, 0, NULL);
				mciSendString(TEXT("stop dead"), NULL, 0, NULL);
				mciSendString(TEXT("close dead"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/dead.wav\" alias dead"), NULL, 0, NULL);
				mciSendString(TEXT("play dead"), NULL, 0, NULL);
				playerHealth = 0;
				glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
			}
		}
		// Check if trap 3 triggered
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(40, 0, 16.75), Vector3f(0.05, 1, 0.75)) && !triggered) {
			triggered = true;
			mciSendString(TEXT("stop trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("close trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("open \"Sounds/trapTrigger.wav\" alias trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("play trapTrigger"), NULL, 0, NULL);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
			glDisable(GL_LIGHT3);
			glDisable(GL_LIGHT4);
			glEnable(GL_LIGHT5);
			loadStaticPositions(1);
			glutTimerFunc(20500, Timer, 14);
		}
		if (trap3) {
			trap3Position.x = trap3Position.x - trap3MoveRate;
			trap3Angle -= 5;
			if (boundingBoxIntersection(playerModelPosition, playerModelRange, trap3Position, trap3Range)) {
				mciSendString(TEXT("stop trap3"), NULL, 0, NULL);
				mciSendString(TEXT("close trap3"), NULL, 0, NULL);
				mciSendString(TEXT("stop dead"), NULL, 0, NULL);
				mciSendString(TEXT("close dead"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/dead.wav\" alias dead"), NULL, 0, NULL);
				mciSendString(TEXT("play dead"), NULL, 0, NULL);
				playerHealth = 0;
				glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
			}
		}
		// Check boss 1 cutscene trigger
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(30.75, 0, 17), Vector3f(0.75, 1, 0.1)) && !boss1FightStart && !boss1CutsceneFinished) {
			savedCameraCenter = camera.center;
			savedCameraEye = camera.eye;
			savedCameraUp = camera.up;
			camera.center = boss1ModelPosition;
			camera.center.y = camera.center.y + 1.5;
			boss1CutsceneView = getVector(playerModelPosition, boss1ModelPosition);
			boss1Cutscene = true;
			mciSendString(TEXT("open \"Sounds/trapTrigger.wav\" alias trapTrigger"), NULL, 0, NULL);
			mciSendString(TEXT("play trapTrigger"), NULL, 0, NULL);
		}
		// Check if fight started with boss 1
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(30.75, 0, 18), Vector3f(0.75, 1, 0.05)) && !boss1FightStart) {
			boss1FightStart = true;
			glutTimerFunc(500, Timer, 1);
			glutTimerFunc(0, Timer, 2);
			loadStaticPositions(1);
		}
		// Check if level 2 needs to start
		if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(5, 0, 30.75), Vector3f(0.05, 1, 0.75)) && !level2CutsceneStarted && !level2CustsceneFinished) {
			level2CutsceneStarted = true;
			savedCameraCenter = camera.center;
			savedCameraEye = camera.eye;
			savedCameraUp = camera.up;
			camera.center = moonPosition;
			camera.eye.y = camera.eye.y + 1;
			mciSendString(TEXT("open \"Sounds/cutscene_2_sound00.wav\" alias cutscene_2_sound00"), NULL, 0, NULL);
			mciSendString(TEXT("play cutscene_2_sound00"), NULL, 0, NULL);
			glutTimerFunc(3500, Timer, 7);
		}
	}
	if (boss1Cutscene) {
		if (camera.eye.z < 34) {
			camera.moveWithTarget(boss1CutsceneView, 0.016);		////////////////////////////////////////////////////
		}
		else {
			if (boss1CutsceneFlag1) {
				if (boss1CutsceneFinished) {
					camera.center = savedCameraCenter;
					camera.up = savedCameraUp;
					camera.eye = savedCameraEye;
					boss1Cutscene = false;
				}
			}
			else {
				boss1CutsceneFlag1 = true;
				glutTimerFunc(2000, Timer, 3);
			}
		}
	}
	if (level2CutsceneStarted && !level2CustsceneFinished) {
		moonRadius += 0.05;
	}
	glutPostRedisplay();
}
void Anim2() {
	// Controls
	if (keyboardBuffer['q']) {
		Vector3f newPos = playerModelPosition;
		newPos.y = newPos.y + moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveY(moveRate);
			movePlayerY(moveRate);
		}
	}
	if (keyboardBuffer['e']) {
		Vector3f newPos = playerModelPosition;
		newPos.y = newPos.y - moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveY(-moveRate);
			movePlayerY(-moveRate);
		}
	}
	if (keyboardBuffer['a']) {
		Vector3f newPos = playerModelPosition;
		Vector3f right = getRightVector();
		right.y = 0;
		newPos = newPos + right * moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveX(moveRate);
			movePlayerX(moveRate);
		}
	}
	if (keyboardBuffer['d']) {
		Vector3f newPos = playerModelPosition;
		Vector3f right = getRightVector();
		right.y = 0;
		newPos = newPos + right * -moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveX(-moveRate);
			movePlayerX(-moveRate);
		}
	}
	if (keyboardBuffer['w']) {
		Vector3f newPos = playerModelPosition;
		Vector3f view = getViewVector();
		view.y = 0;
		newPos = newPos + view * moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveZ(moveRate);
			movePlayerZ(moveRate);
		}
	}
	if (keyboardBuffer['s']) {
		Vector3f newPos = playerModelPosition;
		Vector3f view = getViewVector();
		view.y = 0;
		newPos = newPos + view * -moveRate;
		if (!checkCollision(newPos, playerModelRange)) {
			camera.moveZ(-moveRate);
			movePlayerZ(-moveRate);
		}
	}
	// Player Bullets
	bulletPosition = bulletPosition + bulletDirection.unit() * playerBulletSpeed;
	if (!bossIsHit && boss2Health > 0 && playerHealth > 0) {
		bool bossHit = boundingBoxIntersection(boss2ModelPosition, boss2ModelRange, bulletPosition, bulletRange);
		if (bossHit && boss2FightStart) {
			bossIsHit = true;
			boss2Health--;
			boss2HealthX -= boss2HealthDownRate;
			if (boss2Health == 0) {
				mciSendString(TEXT("stop boss2shots"), NULL, 0, NULL);
				mciSendString(TEXT("close boss2shots"), NULL, 0, NULL);
				mciSendString(TEXT("stop boss2dead"), NULL, 0, NULL);
				mciSendString(TEXT("close boss2dead"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/boss2dead.wav\" alias boss2dead"), NULL, 0, NULL);
				mciSendString(TEXT("play boss2dead"), NULL, 0, NULL);
				mciSendString(TEXT("close survived"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/survived.wav\" alias survived"), NULL, 0, NULL);
				mciSendString(TEXT("play survived"), NULL, 0, NULL);
				glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
			}
			else {
				mciSendString(TEXT("stop bossDamaged"), NULL, 0, NULL);
				mciSendString(TEXT("close bossDamaged"), NULL, 0, NULL);
				mciSendString(TEXT("open \"Sounds/bossDamaged.wav\" alias bossDamaged"), NULL, 0, NULL);
				mciSendString(TEXT("play bossDamaged"), NULL, 0, NULL);
			}
		}
	}
	// Boss 2 Movements & Shootings
	if (boss2Health > 0 && boss2FightStart && playerHealth > 0) {
		if (bossMoveX == 0) {
			if (boss2ModelPosition.x - boss2MoveRate > 10) {
				moveBossX(-boss2MoveRate, 2);
			}
		}
		else {
			if (boss2ModelPosition.x + boss2MoveRate < 70) {
				moveBossX(boss2MoveRate, 2);
			}
		}
		if (bossMoveY == 0) {
			if (boss2ModelPosition.y - boss2MoveRate > 2.75) {
				moveBossY(-boss2MoveRate);
			}
		}
		else {
			if (boss2ModelPosition.y + boss2MoveRate < 20) {
				moveBossY(boss2MoveRate);
			}
		}
		if (bossMoveZ == 0) {
			if (boss2ModelPosition.z - boss2MoveRate > 10) {
				moveBossZ(-boss2MoveRate, 2);

			}
		}
		else {
			if (boss2ModelPosition.z + boss2MoveRate < 70) {
				moveBossZ(boss2MoveRate, 2);
			}
		}
		boss2BulletPosition = boss2BulletPosition + boss2BulletDirection * bossBulletSpeed;
		if (!playerIsHit) {
			bool bossHitPlayer = boundingBoxIntersection(playerModelPosition, playerModelRange, boss2BulletPosition, boss2BulletRange);
			if (bossHitPlayer) {
				playerIsHit = true;
				playerHealth-=3;
				playerHealthY += 3*playerHealthDownRate;
				if (playerHealth <= 0) {
					mciSendString(TEXT("stop dead"), NULL, 0, NULL);
					mciSendString(TEXT("close dead"), NULL, 0, NULL);
					mciSendString(TEXT("open \"Sounds/dead.wav\" alias dead"), NULL, 0, NULL);
					mciSendString(TEXT("play dead"), NULL, 0, NULL);
					glutSetCursor(GLUT_CURSOR_FULL_CROSSHAIR);
				}
				else {
					mciSendString(TEXT("stop playerDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("close playerDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("open \"Sounds/playerDamaged.wav\" alias playerDamaged"), NULL, 0, NULL);
					mciSendString(TEXT("play playerDamaged"), NULL, 0, NULL);
				}
			}
		}
	}
	// Check if fight started with boss 2
	if (boundingBoxIntersection(playerModelPosition, playerModelRange, Vector3f(10, 0, 30.75), Vector3f(0.05, 1, 0.75)) && !boss2FightStart) {
		boss2FightStart = true;
		glutTimerFunc(500, Timer, 5);
		glutTimerFunc(0, Timer, 6);
	}
	if (flashBangHide) {
		flashBangY1 -= flashBangHideRate;
		flashBangY2 += flashBangHideRate;
	}
	glutPostRedisplay();
}

/* Display */
void Display1(void) {
	setupCamera(width, height);
	setupLights();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Environment //
	renderGround(groundTex);
	renderSky(skyTex);
	boss1Health > 0 ? renderMoon(moon1Tex) : renderMoon(moon2Tex);
	renderLevel1Design();

// Player Model //
	if (!cameraMode) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(playerModelPosition.x, playerModelPosition.y + 0.3, playerModelPosition.z);
		glScaled(0.0003, 0.0003, 0.0003);
		glRotatef(playerRotation, 0, 1, 0);
		playerModel.Draw();
		glPopMatrix();
	}
	// Weapon
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(gunModelPosition.x, gunModelPosition.y , gunModelPosition.z);
	glScaled(0.0004, 0.0004, 0.0004);
	//glRotated(weaponRotationX, -1, 0, 0);
	glRotated(playerRotation - 90, 0, 1, 0);
	gunModel.Draw();
	glPopMatrix();

// Boss 1 Model //
	if (boss1Health > 0) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(boss1ModelPosition.x, boss1ModelPosition.y - 1.5, boss1ModelPosition.z);
		glRotatef(boss1Rotation, 0, 1, 0);
		boss1Model.Draw();
		glPopMatrix();
	}
// Traps //
	if (trap1) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(trap1Position.x, trap1Position.y, trap1Position.z);
		glScaled(0.2, 0.2, 0.2);
		glRotated(-110, 0, 1, 0);
		trap1Model.Draw();
		glPopMatrix();
	}
	if (trap2) {
		glPushMatrix();
		glTranslated(trap2Position.x, 0, 0);
		glBindTexture(GL_TEXTURE_2D, trap2Tex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0, 0);glVertex3f(0, 0, 14.5);
		glTexCoord2f(1, 0);glVertex3f(0, 0, 16);
		glTexCoord2f(1, 1);glVertex3f(0, 2, 16);
		glTexCoord2f(0, 1);glVertex3f(0, 2, 14.5);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
	}
	if (trap3) {
		glPushMatrix();
		GLUquadricObj* qobj;
		qobj = gluNewQuadric();
		glTranslated(trap3Position.x, 0.75, trap3Position.z);
		glRotated(-90, 1, 0, 0);
		glRotated(trap3Angle, 0, 0, 1);
		glBindTexture(GL_TEXTURE_2D, trap3Tex.texture[0]);
		gluQuadricTexture(qobj, true);
		gluQuadricNormals(qobj, GL_SMOOTH);
		gluSphere(qobj, 0.75, 40, 40);
		gluDeleteQuadric(qobj);
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
	}
	if (triggered) {
		// 1
		glPushMatrix();
		glColor3d(1, 0, 0);
		glTranslated(35, 0, 7.75);
		glScaled(0.1, 2, 1.5);
		glutSolidCube(1);
		glPopMatrix();
		// 2
		glPushMatrix();
		glColor3d(1, 0, 0);
		glTranslated(35, 0, 15.25);
		glScaled(0.1, 2, 1.5);
		glutSolidCube(1);
		glPopMatrix();
		// 3
		glPushMatrix();
		glColor3d(1, 0, 0);
		glTranslated(33, 0, 16.75);
		glScaled(0.1, 2, 1.5);
		glutSolidCube(1);
		glPopMatrix();
	}
// Bullet //
	// Player
	if (playerHealth > 0) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(bulletPosition.x, bulletPosition.y, bulletPosition.z);
		glutSolidSphere(0.01, 50, 50);
		glPopMatrix();
	}
	// Boss
	if (boss1Health > 0) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(boss1BulletPosition.x, boss1BulletPosition.y, boss1BulletPosition.z);
		glScaled(0.1, 0.1, 0.1);
		treeLogModel.Draw();
		glPopMatrix();
	}
	/*
	Vector3f(5, 0, 8);
	Vector3f(0.05, 1, 4);
	*/

// Test
	/*objectPositions["mountainX+"] = Vector3f(80, 0, 40);
	objectRanges["mountainX+"] = Vector3f(15, 30, 40);

	objectPositions["mountainZ+"] = Vector3f(40, 0, 80);
	objectRanges["mountainZ+"] = Vector3f(40, 15, 20);*/
	// Models
	//glPushMatrix();
	//glColor3f(1, 1, 1);
	//glDisable(GL_LIGHTING);
	//glTranslated(40, 0, 40);
	//glScaled(50, 50, 50);
	//grassModel.Draw();
	//glEnable(GL_LIGHTING);
	//glPopMatrix();

	// Boundry Cube
	/*
	glPushMatrix();
	glTranslated(0, 0, 0);
	glScaled(1, 1, 1);
	glutSolidCube(1);
	glPopMatrix();
	*/

	// Cube +ve Z-Axis
	/*glPushMatrix();
	glColor3f(1, 1, 1);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glBindTexture(GL_TEXTURE_2D, groundTex.texture[0]);
	glTranslated(0, 0.5, 2);
	glutSolidCube(1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glEnable(GL_LIGHTING);
	glPopMatrix();*/

/***************************************************************************************************/
/* 2D Mode Drawings */
	// 2D Setup
	glBindTexture(GL_TEXTURE_2D, 0);
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	char x[50];
	sprintf(x, "Time: %d:%d", minutes, seconds);
	print(640, 700, x);
	// Aim
	if (!boss1Cutscene && playerHealth > 0) {
		glLineWidth(2);
		glBegin(GL_LINES);
		glColor3f(1, 0, 0);
		glVertex2d(width / 2 - 5, height / 2);
		glVertex2d(width / 2 + 5, height / 2);
		glVertex2d(width / 2, height / 2 + 5);
		glVertex2d(width / 2, height / 2 - 5);
		glEnd();
		// Health Bar
		glLineWidth(1.5);
		glBegin(GL_LINE_LOOP);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2f(20, 700);
		glVertex2f(65, 700);
		glVertex2f(65, 400);
		glVertex2f(20, 400);
		glEnd();
		glBegin(GL_QUADS);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2f(20, 700);
		glVertex2f(65, 700);
		glVertex2f(65, playerHealthY);
		glVertex2f(20, playerHealthY);
		glEnd();
		// Boss 1 health bar
		if (boss1FightStart && boss1Health > 0) {
			glBegin(GL_LINE_LOOP);
			glColor3f(1.0f, 1.0f, 1.0f);
			glVertex2f(300, 50);
			glVertex2f(980, 50);
			glVertex2f(980, 100);
			glVertex2f(300, 100);
			glEnd();
			glBegin(GL_QUADS);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex2f(300, 50);
			glVertex2f(boss1HealthX, 50);
			glVertex2f(boss1HealthX, 100);
			glVertex2f(300, 100);
			glEnd();
			glColor3f(1, 1, 1);
			print(90, 75, "Cute Katkot");
		}
	}
	// Game over
	if (playerHealth == 0) {
		glBindTexture(GL_TEXTURE_2D, gameoverTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// Flashbang
	if (flashBangShow) {
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2f(0, 0);
		glVertex2f(width, 0);
		glVertex2f(width, height);
		glVertex2f(0, height);
		glEnd();
	}
	glEnable(GL_LIGHTING);
	glPopMatrix();

/***************************************************************************************************/

	glFlush();
}
void Display2(void) {
	setupCamera(width, height);
	setupLights();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Sky & Ground //
	renderGround(ground2Tex);
	renderSky(sky2Tex);

// Game Boundries
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(130, 0, 50);
	glScaled(0.1, 0.05, 0.08);
	castleModel.Draw();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(40, 0, 80);
	glScaled(15, 10, 5);
	mountainModel.Draw();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(40, 0, 0);
	glRotated(180, 0, 1, 0);
	glScaled(15, 10, 5);
	mountainModel.Draw();
	glPopMatrix();

	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(-20, 0, 40);
	glRotated(90, 0, 1, 0);
	glScaled(15, 10, 5);
	mountainModel.Draw();
	glPopMatrix();

// Player Model
	if (!cameraMode) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(playerModelPosition.x, playerModelPosition.y + 0.3, playerModelPosition.z);
		glScaled(0.0003, 0.0003, 0.0003);
		glRotatef(playerRotation, 0, 1, 0);
		playerModel.Draw();
		glPopMatrix();
	}
	// Weapon
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(gunModelPosition.x, gunModelPosition.y, gunModelPosition.z);
	glScaled(0.0004, 0.0004, 0.0004);
	//glRotated(weaponRotationX, -1, 0, 0);
	glRotated(playerRotation - 90, 0, 1, 0);
	gunModel.Draw();
	glPopMatrix();

// Boss 2 Model //
	if (boss2Health > 0) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(boss2ModelPosition.x, boss2ModelPosition.y -2.75, boss2ModelPosition.z);
		glScaled(0.2, 0.2, 0.2);
		glRotatef(boss2Rotation, 0, 1, 0);
		dragonModel.Draw();
		glPopMatrix();
	}

// Bullet //
	// Player
	if (playerHealth > 0) {
		glPushMatrix();
		glColor3f(1, 1, 1);
		glTranslated(bulletPosition.x, bulletPosition.y, bulletPosition.z);
		glutSolidSphere(0.01, 50, 50);
		glPopMatrix();
	}
	// Boss
	if (boss2Health > 0) {
		glPushMatrix();
		glDisable(GL_LIGHTING);
		GLUquadricObj* qobj;
		qobj = gluNewQuadric();
		glTranslated(boss2BulletPosition.x, boss2BulletPosition.y, boss2BulletPosition.z);
		glRotated(bossBulletAngle, 1, 0, 0);
		glBindTexture(GL_TEXTURE_2D, fireTex.texture[0]);
		gluQuadricTexture(qobj, true);
		gluQuadricNormals(qobj, GL_SMOOTH);
		gluSphere(qobj, 0.1, 40, 40);
		gluDeleteQuadric(qobj);
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}

// Scene Boundires
	// 28
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(5, 0, 30);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();
	// 29
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslated(5, 0, 31.5);
	glScaled(0.04, 0.02, 0.006);
	glRotated(-90, 0, 0, 1);
	wall0Model.Draw();
	glPopMatrix();

/***************************************************************************************************/

/* 2D Mode Drawings */
	// 2D Setup
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_LIGHTING);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	char x[50];
	sprintf(x, "Time: %d:%d", minutes, seconds);
	print(640, 700, x);
	if (playerHealth > 0 && boss2Health > 0) {
		// Aim
		glLineWidth(2);
		glBegin(GL_LINES);
		glColor3f(1, 0, 0);
		glVertex2d(width / 2 - 5, height / 2);
		glVertex2d(width / 2 + 5, height / 2);
		glVertex2d(width / 2, height / 2 + 5);
		glVertex2d(width / 2, height / 2 - 5);
		glEnd();
		// Health Bar
		glLineWidth(1.5);
		glBegin(GL_LINE_LOOP);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2f(20, 700);
		glVertex2f(65, 700);
		glVertex2f(65, 400);
		glVertex2f(20, 400);
		glEnd();
		glBegin(GL_QUADS);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex2f(20, 700);
		glVertex2f(65, 700);
		glVertex2f(65, playerHealthY);
		glVertex2f(20, playerHealthY);
		glEnd();
		// Boss 2 health bar
		if (boss2FightStart && boss2Health > 0) {
			glBegin(GL_LINE_LOOP);
			glColor3f(1.0f, 1.0f, 1.0f);
			glVertex2f(300, 50);
			glVertex2f(980, 50);
			glVertex2f(980, 100);
			glVertex2f(300, 100);
			glEnd();
			glBegin(GL_QUADS);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex2f(300, 50);
			glVertex2f(boss2HealthX, 50);
			glVertex2f(boss2HealthX, 100);
			glVertex2f(300, 100);
			glEnd();
			glColor3f(1, 1, 1);
			print(90, 75, "Big Boss");
		}
	}
	// Game over
	if (playerHealth == 0) {
		glBindTexture(GL_TEXTURE_2D, gameoverTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// Game finished
	if (boss2Health == 0) {
		glBindTexture(GL_TEXTURE_2D, survivedTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	// Flashbang
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0, 0);
	glVertex2f(width, 0);
	glVertex2f(width, flashBangY1);
	glVertex2f(0, flashBangY1);
	glEnd();
	glBegin(GL_QUADS);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(0, flashBangY2);
	glVertex2f(width, flashBangY2);
	glVertex2f(width, height);
	glVertex2f(0, height);
	glEnd();
	glEnable(GL_LIGHTING);
	glPopMatrix();

/***************************************************************************************************/

	glFlush();
}
void Display3(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_LIGHTING);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Game finished
	if (start) {
		glBindTexture(GL_TEXTURE_2D, startTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (controls) {
		glBindTexture(GL_TEXTURE_2D, controlsTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (about) {
		glBindTexture(GL_TEXTURE_2D, aboutTex.texture[0]);
		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);glVertex2f(0, height);
		glTexCoord2f(1.0f, 0.0f);glVertex2f(width, height);
		glTexCoord2f(1.0f, 1.0f);glVertex2f(width, 0);
		glTexCoord2f(0.0f, 1.0f);glVertex2f(0, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glEnable(GL_LIGHTING);
	glPopMatrix();

	glFlush();
}

/* Main */
void main(int argc, char** argv) {
	glutInit(&argc, argv);

	glutInitWindowSize(width, height);
	glutInitWindowPosition(windowPosX, windowPosY);

	glutCreateWindow("House of the MemeZ");
	glutDisplayFunc(Display3);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	glutIgnoreKeyRepeat(1);
	glutSpecialFunc(Special);
	glutSpecialUpFunc(SpecialUp);
	glutMouseFunc(Mouse);
	glutMotionFunc(MouseClickMotion);
	glutPassiveMotionFunc(MouseMotion);
	glutReshapeFunc(Reshape);
	//glutTimerFunc(20000, Timer, 9);
	//glutTimerFunc(0, Timer, 16);
	loadModels();
	loadStaticPositions(1);
	glutSetCursor(GLUT_CURSOR_CROSSHAIR);
	mciSendString(TEXT("open \"Sounds/introMusic.wav\" alias introMusic"), NULL, 0, NULL);
	mciSendString(TEXT("play introMusic"), NULL, 0, NULL);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);

	glutMainLoop();
}
