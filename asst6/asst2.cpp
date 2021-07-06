////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <list>
#include <fstream>
#include <iostream>

#include <GL/glew.h>
#ifdef __APPLE__
#   include <GLUT/glut.h>
#else
#   include <GL/glut.h>
#endif

#include "cvec.h"
#include "matrix4.h"
#include "geometrymaker.h"
#include "ppm.h"
#include "glsupport.h"
#include "rigtform.h"
#include "arcball.h"
#include "asstcommon.h"
#include "scenegraph.h"
#include "drawer.h"
#include "picker.h"
#include "sgutils.h"
#include "geometry.h"

using namespace std;      // for string, vector, iostream, and other standard C++ stuff

// G L O B A L S ///////////////////////////////////////////////////

// --------- IMPORTANT --------------------------------------------------------
// Before you start working on this assignment, set the following variable
// properly to indicate whether you want to use OpenGL 2.x with GLSL 1.0 or
// OpenGL 3.x+ with GLSL 1.3.
//
// Set g_Gl2Compatible = true to use GLSL 1.0 and g_Gl2Compatible = false to
// use GLSL 1.3. Make sure that your machine supports the version of GLSL you
// are using. In particular, on Mac OS X currently there is no way of using
// OpenGL 3.x with GLSL 1.3 when GLUT is used.
//
// If g_Gl2Compatible=true, shaders with -gl2 suffix will be loaded.
// If g_Gl2Compatible=false, shaders with -gl3 suffix will be loaded.
// To complete the assignment you only need to edit the shader files that get
// loaded
// ----------------------------------------------------------------------------
// remove: static const bool g_Gl2Compatible = false;
const bool g_Gl2Compatible = false;


static const float g_frustMinFov = 60.0;  // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;    // near plane
static const float g_frustFar = -50.0;    // far plane
static const float g_groundY = -2.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static bool g_mouseClickDown = false;    // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

//static int g_activeObj = 0;
static int g_viewPoint = 0;
static RigTForm g_eyeRbt;
//static int g_worldSky = 0;
static int g_arballScreenRadius = 1;
static double g_arcballScale = 0.0078125;
static RigTForm g_arcballRbt;
static bool g_picking = false;
static bool g_drawArc = true;
static list<vector<RigTForm>> keyframes;
static list<vector<RigTForm>>::iterator curKeyframe;
static vector<shared_ptr<SgRbtNode>> curRbtnodes;
static int index;
static string filename = "animation.txt";
static int g_msBetweenKeyFrames = 2000;
static int g_animateFramesPerSecond = 60;
static bool g_animateFlag = true;
static vector<RigTForm> curFrame;
static vector<RigTForm> nextFrame;

static shared_ptr<Material> g_redDiffuseMat,
							g_blueDiffuseMat,
							g_bumpFloorMat,
							g_arcballMat,
							g_pickingMat,
							g_lightMat;

shared_ptr<Material> g_overridingMaterial;

// --------- Geometry
typedef SgGeometryShapeNode MyShapeNode;

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube, g_sphere;

static shared_ptr<SgRootNode> g_world;
static shared_ptr<SgRbtNode> g_skyNode, g_groundNode, g_robot1Node, g_robot2Node;
static shared_ptr<SgRbtNode> g_currentPickedRbtNode, g_eyeRbtNode; // used later when you do picking
static shared_ptr<SgRbtNode> g_light1Node, g_light2Node;

// --------- Scene

static const Cvec3 g_light1(2.0, 3.0, 14.0), g_light2(-2, -3.0, -5.0);  // define two lights positions in world space
static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.25, 4.0));
static RigTForm g_objectRbt = RigTForm(Cvec3(-1, 0, 0));  // currently only 1 obj is defined
static RigTForm g_objectRbt2 = RigTForm(Cvec3(1, 0, 0));
static Cvec3f g_objectColors[1] = { Cvec3f(1, 0, 0) };
static Cvec3f g_objectColors2[1] = { Cvec3f(0, 1, 0) };

///////////////// END OF G L O B A L S //////////////////////////////////////////////////




static void initGround() {
	int ibLen, vbLen;
	getPlaneVbIbLen(vbLen, ibLen);

	// Temporary storage for cube Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makePlane(g_groundSize * 2, vtx.begin(), idx.begin());
	g_ground.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initCubes() {
	int ibLen, vbLen;
	getCubeVbIbLen(vbLen, ibLen);

	// Temporary storage for cube Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);

	makeCube(1, vtx.begin(), idx.begin());
	g_cube.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vbLen, ibLen));
}

static void initSphere() {
	int ibLen, vbLen;
	getSphereVbIbLen(20, 10, vbLen, ibLen);

	// Temporary storage for sphere Geometry
	vector<VertexPNTBX> vtx(vbLen);
	vector<unsigned short> idx(ibLen);
	makeSphere(1, 20, 10, vtx.begin(), idx.begin());
	g_sphere.reset(new SimpleIndexedGeometryPNTBX(&vtx[0], &idx[0], vtx.size(), idx.size()));
}

inline void sendProjectionMatrix(Uniforms& uniforms, const Matrix4& projMatrix) {
	uniforms.put("uProjMatrix", projMatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
	if (g_windowWidth >= g_windowHeight)
		g_frustFovY = g_frustMinFov;
	else {
		const double RAD_PER_DEG = 0.5 * CS175_PI / 180;
		g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight / g_windowWidth, cos(g_frustMinFov * RAD_PER_DEG)) / RAD_PER_DEG;
	}
}

static Matrix4 makeProjectionMatrix() {
	return Matrix4::makeProjection(
		g_frustFovY, g_windowWidth / static_cast <double> (g_windowHeight),
		g_frustNear, g_frustFar);
}

static void drawStuff(bool picking) {
	// Declare an empty uniforms
	Uniforms uniforms;
	// build & send proj. matrix to vshader
	const Matrix4 projmat = makeProjectionMatrix();
	sendProjectionMatrix(uniforms, projmat);

	// use the skyRbt as the eyeRbt
	//cout << "getPathAccumRbt for eye" << endl;
	if (g_viewPoint == 1) {
		g_eyeRbtNode = g_robot1Node;
	}
	else if (g_viewPoint == 2) {
		g_eyeRbtNode = g_robot2Node;
	}
	else {
		g_eyeRbtNode = g_skyNode;
	}
	g_eyeRbt = getPathAccumRbt(g_world, g_eyeRbtNode);
	RigTForm eyeRbt = g_eyeRbt;
	const RigTForm invEyeRbt = inv(eyeRbt);

	Cvec3 light1 = getPathAccumRbt(g_world, g_light1Node).getTranslation();
	uniforms.put("uLight", Cvec3(invEyeRbt * Cvec4(light1, 1)));
	Cvec3 light2 = getPathAccumRbt(g_world, g_light2Node).getTranslation();
	uniforms.put("uLight2", Cvec3(invEyeRbt * Cvec4(light2, 1)));

	if (!picking) {
		Drawer drawer(invEyeRbt, uniforms);
		g_world->accept(drawer);

		// draw arcball
		g_drawArc = true;
		if (g_viewPoint != 0) {
			if (g_viewPoint == 1 && (g_currentPickedRbtNode == g_robot1Node || g_currentPickedRbtNode == shared_ptr<SgRbtNode>())) {
				//cout << "do not draw arc" << endl;
				g_drawArc = false;
			}
			else if (g_viewPoint == 2 && (g_currentPickedRbtNode == g_robot2Node || g_currentPickedRbtNode == shared_ptr<SgRbtNode>())) {
				//cout << "do not draw arc" << endl;
				g_drawArc = false;
			}
		}
		if (g_drawArc) {
			//cout << "getPathAccumRbt for arcball" << endl;
			g_arcballRbt = getPathAccumRbt(g_world, g_currentPickedRbtNode);
			Matrix4 MVM = rigTFormToMatrix(invEyeRbt) * rigTFormToMatrix(g_arcballRbt);
			if (!(g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton))) {
				if (MVM(2, 3) <= -CS175_EPS) {
					g_arcballScale = getScreenToEyeScale(MVM(2, 3), g_frustFovY, g_windowHeight);
				}
			}
			double radius = g_arballScreenRadius * g_arcballScale;
			//cout << "g_arcballScale " << getScreenToEyeScale(MVM(2,3), g_frustFovY, g_windowHeight) << endl;
			MVM = MVM * Matrix4().makeScale(Cvec3(radius, radius, radius));
			Matrix4 NMVM = normalMatrix(MVM);
			sendModelViewNormalMatrix(uniforms, MVM, NMVM);
			g_arcballMat->draw(*g_sphere, uniforms);
		}
	}
	else {
		//cout << "picking?" << endl;
		Picker picker(invEyeRbt, uniforms);
		g_overridingMaterial = g_pickingMat;
		g_world->accept(picker);
		g_overridingMaterial.reset();
		glFlush();
		g_currentPickedRbtNode = picker.getRbtNodeAtXY(g_mouseClickX, g_mouseClickY);
		if (g_currentPickedRbtNode == g_groundNode) {
			//cout << "pick ground!" << endl;
			g_currentPickedRbtNode = shared_ptr<SgRbtNode>();   // set to NULL
		}
	}

}

static void display() {
	// No more glUseProgram

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawStuff(false); // no more curSS

	glutSwapBuffers();

	checkGlErrors();
}

static void reshape(const int w, const int h) {
	g_windowWidth = w;
	g_windowHeight = h;
	g_arballScreenRadius = 0.25 * min(w, h);
	glViewport(0, 0, w, h);
	cerr << "Size of window is now " << w << "x" << h << endl;
	updateFrustFovY();
	glutPostRedisplay();
}

static void motion(const int x, const int y) {
	if (!g_animateFlag) {
		cout << "Cannot operate when animating!" << endl;
		return;
	}
	const double dx = x - g_mouseClickX;
	const double dy = g_windowHeight - y - 1 - g_mouseClickY;

	Cvec2 o = getScreenSpaceCoord((inv(g_eyeRbt) * g_arcballRbt).getTranslation(), makeProjectionMatrix(), g_frustNear, g_frustFovY, g_windowWidth, g_windowHeight);
	//cout << "arcball origin space coord " << o[0] << "," << o[1] << endl;

	RigTForm m;
	//cout << "g_arballScale: " << g_arcballScale << endl;
	if (g_mouseLClickButton && !g_mouseRClickButton) { // left button down?
		if (!g_drawArc) { // basic rotation when view=active in cube and sky-sky
			Quat q;
			m.setRotation(q.makeXRotation(-dy) * q.makeYRotation(dx));
		}
		else { // arcball rotation
			Cvec3 p1, p2;
			p1[0] = g_mouseClickX;
			p1[1] = g_mouseClickY;
			if (pow(g_arballScreenRadius, 2) - pow(p1[0] - o[0], 2) - pow(p1[1] - o[1], 2) < 0) {
				//cout << "mouse out of arcball" << endl;
				double theta = atan2(p1[1] - o[1], p1[0] - o[0]);
				p1[0] = g_arballScreenRadius * cos(theta) + o[0];
				p1[1] = g_arballScreenRadius * sin(theta) + o[1];
				p1[2] = 0;
			}
			else {
				p1[2] = sqrt(pow(g_arballScreenRadius, 2) - pow(p1[0] - o[0], 2) - pow(p1[1] - o[1], 2));
			}

			p2[0] = x;
			p2[1] = g_windowHeight - y - 1;
			if (pow(g_arballScreenRadius, 2) - pow(p2[0] - o[0], 2) - pow(p2[1] - o[1], 2) < 0) {
				//cout << "mouse out of arcball" << endl;
				double theta = atan2(p2[1] - o[1], p2[0] - o[0]);
				p2[0] = g_arballScreenRadius * cos(theta) + o[0];
				p2[1] = g_arballScreenRadius * sin(theta) + o[1];
				p2[2] = 0;
			}
			else {
				p2[2] = sqrt(pow(g_arballScreenRadius, 2) - pow(p2[0] - o[0], 2) - pow(p2[1] - o[1], 2));
			}
			Cvec3 v1 = normalize(Cvec3(p1[0] - o[0], p1[1] - o[1], p1[2]));
			Cvec3 v2 = normalize(Cvec3(p2[0] - o[0], p2[1] - o[1], p2[2]));
			m.setRotation(Quat(0, v2) * Quat(0, -v1));
		}
	}
	else if (g_mouseRClickButton && !g_mouseLClickButton) { // right button down?
		m.setTranslation(Cvec3(dx, dy, 0) * g_arcballScale);
	}
	else if (g_mouseMClickButton || (g_mouseLClickButton && g_mouseRClickButton)) {  // middle or (left and right) button down?
		m.setTranslation(Cvec3(0, 0, -dy) * g_arcballScale);
	}

	RigTForm A;
	if (g_mouseClickDown) {
		if (g_viewPoint == 0) {
			if (g_currentPickedRbtNode == shared_ptr<SgRbtNode>()) {
				A.setRotation(g_eyeRbt.getRotation());
				g_skyNode->setRbt(A * inv(m) * inv(A) * g_skyNode->getRbt());
			}
			else {
				A.setTranslation(getPathAccumRbt(g_world, g_currentPickedRbtNode).getTranslation());
				A.setRotation(g_eyeRbt.getRotation());
				RigTForm As = inv(getPathAccumRbt(g_world, g_currentPickedRbtNode, 1)) * A;
				g_currentPickedRbtNode->setRbt(As * m * inv(As) * g_currentPickedRbtNode->getRbt());
			}
		}
		else {
			if ((g_viewPoint == 1 && g_currentPickedRbtNode == g_robot1Node) || (g_viewPoint == 2 && g_currentPickedRbtNode == g_robot2Node)
				|| g_currentPickedRbtNode == shared_ptr<SgRbtNode>()) {
				//cout << "eye = robot, pick = own's torso" << endl;
				m.setRotation(inv(m.getRotation()));
				g_eyeRbtNode->setRbt(g_eyeRbtNode->getRbt() * m);
			}
			else {
				A.setTranslation(getPathAccumRbt(g_world, g_currentPickedRbtNode).getTranslation());
				A.setRotation(g_eyeRbt.getRotation());
				RigTForm As = inv(getPathAccumRbt(g_world, g_currentPickedRbtNode, 1)) * A;
				g_currentPickedRbtNode->setRbt(As * m * inv(As) * g_currentPickedRbtNode->getRbt());
			}
		}
		glutPostRedisplay(); // we always redraw if we changed the scene
	}
	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;
}

static void pick() {
	// We need to set the clear color to black, for pick rendering.
	// so let's save the clear color
	GLdouble clearColor[4];
	glGetDoublev(GL_COLOR_CLEAR_VALUE, clearColor);

	glClearColor(0, 0, 0, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// No more glUseProgram
	drawStuff(true); // no more curSS

	// Uncomment below and comment out the glutPostRedisplay in mouse(...) call back
	// to see result of the pick rendering pass
	// glutSwapBuffers();

	//Now set back the clear color
	glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

	checkGlErrors();
}

static void mouse(const int button, const int state, const int x, const int y) {
	if (!g_animateFlag) {
		cout << "Cannot operate when animating!" << endl;
		return;
	}
	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;  // conversion from GLUT window-coordinate-system to OpenGL window-coordinate-system

	g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
	g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
	g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

	g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
	g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
	g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

	g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;
	if (g_picking) {
		pick();
		cout << "picking mode is off" << endl;
		g_picking = false;
	}
	glutPostRedisplay();
}

static void printPtrvector(vector<shared_ptr<SgRbtNode>> vpr) {
	vector<shared_ptr<SgRbtNode>>::iterator iter;
	int cnt = 0;
	for (iter = vpr.begin(); iter != vpr.end(); ++iter) {
		shared_ptr<SgRbtNode> node = *iter;
		RigTForm A = node->getRbt();
		Cvec3 t = A.getTranslation();
		Quat q = A.getRotation();
		cout << cnt << "th Rbt: t[" << t[0] << " " << t[1] << " " << t[2] << "] q[" << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << "]" << endl;
		cnt++;
	}
}

static void printVector(vector<RigTForm> vr) {
	vector<RigTForm>::iterator iter;
	int cnt = 0;
	for (iter = vr.begin(); iter != vr.end(); ++iter) {
		RigTForm A = *iter;
		Cvec3 t = A.getTranslation();
		Quat q = A.getRotation();
		cout << cnt << "th Rbt: t[" << t[0] << " " << t[1] << " " << t[2] << "] q[" << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << "]" << endl;
		cnt++;
	}
}

static void printList(list<vector<RigTForm>> lvr) {
	cout << "---------------------printList---------------------------" << endl;
	list<vector<RigTForm>>::iterator iter;
	int cnt = 0;
	for (iter = lvr.begin(); iter != lvr.end(); ++iter) {
		cout << cnt << "th keyframe" << endl;
		printVector(*iter);
		cout << "/////////////////////////////////////////////////////////" << endl;
		cnt++;
	}
}

static vector<RigTForm> makeKeyframe(vector<shared_ptr<SgRbtNode>> c) {
	vector<RigTForm> result;
	vector<shared_ptr<SgRbtNode>>::iterator iter;
	for (iter = c.begin(); iter != c.end(); ++iter) {
		shared_ptr<SgRbtNode> node = *iter;
		result.push_back(node->getRbt());
	}
	return result;
}

static void updateNode(vector<RigTForm> keyframe) {
	vector<RigTForm>::iterator iter = keyframe.begin();
	vector<shared_ptr<SgRbtNode>>::iterator piter = curRbtnodes.begin();
	while (iter != keyframe.end()) {
		RigTForm rbt = *iter;
		shared_ptr<SgRbtNode> node = *piter;
		node->setRbt(rbt);
		++iter;
		++piter;
	}
}

bool interpolateAndDisplay(float t) {
	//cout << t << endl;
	if (t < keyframes.size() - 3) {
		float a = t - (int)t;
		//cout << a << endl;
		if (t >= index) {
			cout << "Animate frame[" << index + 1 <<"] to frame[" << index + 2 << "]" << endl;
			curFrame = *curKeyframe;
			++curKeyframe;
			nextFrame = *curKeyframe;
			++index;
		}
		vector<RigTForm>::iterator citer = curFrame.begin(), niter = nextFrame.begin();
		vector<RigTForm> intFrame;
		while (citer != curFrame.end()) {
			RigTForm intRBT = linInterpolation(*citer, *niter, a);
			intFrame.push_back(intRBT);
			++citer;
			++niter;
		}
		updateNode(intFrame);
		return false;
	}
	cout << "Animation end" << endl;
	return true;
}

static void animateTimerCallback(int ms) {
	//cout << "AnimateCallback" << endl;
	float t = (float)ms / (float)g_msBetweenKeyFrames;
	bool endReached = interpolateAndDisplay(t);
	if (!endReached && !g_animateFlag) {
		glutTimerFunc(1000 / g_animateFramesPerSecond, animateTimerCallback, ms + 1000 / g_animateFramesPerSecond);
	}
	else {
		g_animateFlag = true;
	}
	glutPostRedisplay();
}

static void keyboard(const unsigned char key, const int x, const int y) {
	vector<RigTForm> keyframe;
	ofstream ofstr;
	ifstream ifstr;
	int numKeyframes, numRbtNodes;

	switch (key) {
	case 27:
		exit(0);                                  // ESC
	case 'h':
		cout << " ============== H E L P ==============\n\n"
			<< "h\t\thelp menu\n"
			<< "s\t\tsave screenshot\n"
			//<< "f\t\tToggle flat shading on/off.\n"
			<< "p\t\tUse mouse to pick a part to edit\n"
			<< "v\t\tCycle view\n"
			<< "space\t\tCopy current RBT to keyframe\n"
			<< "u\t\tUpdate RBT to current keyframe\n"
			<< ">\t\tAdvance to next keyframe\n"
			<< "<\t\tRetreat to previous keyframe\n"
			<< "d\t\tDelete current keyframe\n"
			<< "n\t\tCreate new keyframe\n"
			<< "i\t\tInput keyframes from input file\n"
			<< "w\t\tOutput keyframes to output file\n"
			<< "y\t\tPlay/stop the animation\n"
			<< "+\t\tMake the animation go faster\n"
			<< "-\t\tMake the animation go slower\n"
			<< "drag left mouse to rotate\n" << endl;
		break;
	case 's':
		glFlush();
		writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
		break;
	/*case 'f':
		g_activeShader ^= 1;
		break;*/
	case 'v':
		g_viewPoint++;
		if (g_viewPoint == 1) {
			cout << "Active eye is Robot1" << endl;
		}
		else if (g_viewPoint == 2) {
			cout << "Active eye is Robot2" << endl;
		}
		else {
			g_viewPoint = 0;
			cout << "Active eye is Sky" << endl;
		}
		break;
	case 'p':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		if (!g_picking) {
			cout << "picking mode is on" << endl;
			g_picking = true;
		}
		else {
			cout << "picking mode is off" << endl;
			g_picking = false;
		}
		break;
	case ' ':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		if (keyframes.size() == 0) {
			cout << "No key frame defined" << endl;
		}
		else {
			updateNode(*curKeyframe);
			cout << "Loading current key frame[" << index + 1 << "] to scene graph" << endl;
		}
		break;
	case 'u':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		keyframe = makeKeyframe(curRbtnodes);
		if (keyframes.size() != 0) {
			cout << "Copying scene graph to current frame[" << index + 1 << "]" <<  endl;
			*curKeyframe = keyframe;
		}
		else {
			keyframes.push_back(keyframe);
			curKeyframe = keyframes.begin();
			index = -1;
			cout << "Create new keyframe[" << index + 1 << "]" << endl;
		}
		break;
	case '<':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		if (keyframes.size() != 0) {
			if (curKeyframe == keyframes.begin()) {
				cout << "First keyframe!!" << endl;
			}
			else {
				--curKeyframe;
				--index;
				updateNode(*curKeyframe);
				cout << "Stepped backward frame[" << index + 1 << "]" << endl;
			}
		}
		break;
	case '>': 
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		if (keyframes.size() != 0) {
			if (curKeyframe == --keyframes.end()) {
				cout << "Last key frame!!" << endl;
			}
			else {
				++curKeyframe;
				++index;
				updateNode(*curKeyframe);
				cout << "Stepped forward frame[" << index + 1 << "]" << endl;
			}
		}
		break;
	case 'd':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		if (keyframes.size() != 0) {
			cout << "Deleting current frame [" << index + 1 << "]" << endl;
			curKeyframe = keyframes.erase(curKeyframe);
			if (keyframes.size() != 0) {
				if (curKeyframe != keyframes.begin()) {
					--curKeyframe;
					--index;
					updateNode(*curKeyframe);
					cout << "Now at frame [" << index + 1 << "]" << endl;
				}
				else {
					updateNode(*curKeyframe);
					cout << "Now at frame [" << index + 1 << "]" << endl;
				}
			}
			else {
				cout << "No key frame defined" << endl;
			}
		}
		break;
	case 'n':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		keyframe = makeKeyframe(curRbtnodes);
		//printVector(keyframe);
		if (keyframes.size() != 0) {
			curKeyframe = keyframes.insert(++curKeyframe, keyframe);
			++index;
			cout << "Create new frame[" << index + 1 << "]" << endl;
		}
		else {
			keyframes.push_back(keyframe);
			curKeyframe = keyframes.begin();
			index = -1;
			cout << "Create new frame[" << index + 1 << "]" << endl;
		}
		break;
	case 'i':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		ifstr.open(filename, ios::binary);
		cout << "Reading animation from " << filename << endl;
		ifstr >> numKeyframes >> numRbtNodes;
		cout << numKeyframes << " frames read. "  << numRbtNodes <<  endl;
		keyframes.clear();
		for (int i = 0; i < numKeyframes; i++) {
			for (int j = 0; j < numRbtNodes; j++) {
				Cvec3 t;
				Quat q;
				ifstr >> t[0]  >> t[1]  >> t[2]  >> q[0]  >> q[1]  >> q[2] >> q[3];
				//cout << t[0] << ' ' << t[1] << ' ' << t[2] << ' ' << q[0] << ' ' << q[1] << ' ' << q[2] << ' ' << q[3] << '\n';
				RigTForm rbt = RigTForm(t, q);
				keyframe.push_back(rbt);
			}
			keyframes.push_back(keyframe);
			keyframe.clear();
		}
		//printList(keyframes);
		curKeyframe = keyframes.begin();
		index = -1;
		updateNode(*curKeyframe);
		ifstr.close();
		break;
	case 'w':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		ofstr.open(filename, ios::binary);
		cout << "Writing animation to "  << filename << endl;
		ofstr << keyframes.size() << ' ' << curRbtnodes.size() << '\n';
		for (list<vector<RigTForm>>::iterator iter = keyframes.begin(); iter != keyframes.end(); ++iter) {
			for (vector<RigTForm>::iterator jter = (*iter).begin(); jter != (*iter).end(); ++jter) {
				Cvec3 t = (*jter).getTranslation();
				Quat q = (*jter).getRotation();
				ofstr << t[0] << ' ' << t[1] << ' ' << t[2] << ' ' << q[0] << ' ' << q[1] << ' ' << q[2] << ' ' << q[3] << '\n';
			}
		}
		ofstr.close();
		break;
	case 'y':
		//cout << "Current Time(ms): " << g_Time << endl;
		if (keyframes.size() <= 3) {
			cout << "Cannot play animation with less than 4 keyframes. Current keyframes: " << keyframes.size() << endl;
		}
		else {
			if (g_animateFlag) {
				curKeyframe = keyframes.begin();
				++curKeyframe;
				index = 0;
				g_animateFlag = false;
				cout << "Play from frame[1]!" << endl;
				animateTimerCallback(0);
			}
			else {
				g_animateFlag = true;
				cout << "Stop" << endl;
			}
		}
		break;
	case '+':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break;
		}
		g_msBetweenKeyFrames -= 100;
		cout << g_msBetweenKeyFrames << "ms between keyframes" << endl;
		break;
	case '-':
		if (!g_animateFlag) {
			cout << "Cannot operate when animating!" << endl;
			break; 
		}
		g_msBetweenKeyFrames += 100;
		cout << g_msBetweenKeyFrames << "ms between keyframes" << endl;
		break;
	}
	//printList(keyframes);
	glutPostRedisplay();
}

static void initGlutState(int argc, char* argv[]) {
	glutInit(&argc, argv);                                  // initialize Glut based on cmd-line args
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);  //  RGBA pixel channels and double buffering
	glutInitWindowSize(g_windowWidth, g_windowHeight);      // create a window
	glutCreateWindow("Assignment 2");                       // title the window

	glutDisplayFunc(display);                               // display rendering callback
	glutReshapeFunc(reshape);                               // window reshape callback
	glutMotionFunc(motion);                                 // mouse movement callback
	glutMouseFunc(mouse);                                   // mouse click callback
	glutKeyboardFunc(keyboard);
}

static void initGLState() {
	glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);
	glClearDepth(0.);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glReadBuffer(GL_BACK);
	if (!g_Gl2Compatible)
		glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initMaterials() {
	// Create some prototype materials
	Material diffuse("./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader");
	Material solid("./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader");

	// copy diffuse prototype and set red color
	g_redDiffuseMat.reset(new Material(diffuse));
	g_redDiffuseMat->getUniforms().put("uColor", Cvec3f(1, 0, 0));

	// copy diffuse prototype and set blue color
	g_blueDiffuseMat.reset(new Material(diffuse));
	g_blueDiffuseMat->getUniforms().put("uColor", Cvec3f(0, 0, 1));

	// normal mapping material
	g_bumpFloorMat.reset(new Material("./shaders/normal-gl3.vshader", "./shaders/normal-gl3.fshader"));
	g_bumpFloorMat->getUniforms().put("uTexColor", shared_ptr<ImageTexture>(new ImageTexture("Fieldstone.ppm", true)));
	g_bumpFloorMat->getUniforms().put("uTexNormal", shared_ptr<ImageTexture>(new ImageTexture("FieldstoneNormal.ppm", false)));

	// copy solid prototype, and set to wireframed rendering
	g_arcballMat.reset(new Material(solid));
	g_arcballMat->getUniforms().put("uColor", Cvec3f(0.27f, 0.82f, 0.35f));
	g_arcballMat->getRenderStates().polygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// copy solid prototype, and set to color white
	g_lightMat.reset(new Material(solid));
	g_lightMat->getUniforms().put("uColor", Cvec3f(1, 1, 1));

	// pick shader
	g_pickingMat.reset(new Material("./shaders/basic-gl3.vshader", "./shaders/pick-gl3.fshader"));
};


static void initGeometry() {
	initGround();
	initCubes();
	initSphere();
}

static void constructRobot(shared_ptr<SgTransformNode> base, shared_ptr<Material> material) {

	const double ARM_LEN = 0.7,
		ARM_THICK = 0.25,
		TORSO_LEN = 1.5,
		TORSO_THICK = 0.25,
		TORSO_WIDTH = 1;
	const int NUM_JOINTS = 10,
		NUM_SHAPES = 10;

	struct JointDesc {
		int parent;
		float x, y, z;
	};

	JointDesc jointDesc[NUM_JOINTS] = {
	  {-1}, // torso
	  {0,  TORSO_WIDTH / 2, TORSO_LEN / 2, 0}, // upper right arm
	  {1,  ARM_LEN, 0, 0}, // lower right arm
	  {0,  -TORSO_WIDTH / 2, TORSO_LEN / 2, 0}, // upper left arm
	  {3,  -ARM_LEN, 0, 0}, // lower left arm
	  {0,  TORSO_LEN / 4, -TORSO_LEN / 2, 0}, // upper right leg
	  {5,  0, -TORSO_WIDTH, 0}, // lower right leg
	  {0,  -TORSO_LEN / 4, -TORSO_LEN / 2, 0}, // upper left leg
	  {7,  0, -TORSO_WIDTH, 0}, // lower left leg
	  {0, 0, TORSO_LEN / 2, 0}, // head
	};

	struct ShapeDesc {
		int parentJointId;
		float x, y, z, sx, sy, sz;
		shared_ptr<Geometry> geometry;
	};

	ShapeDesc shapeDesc[NUM_SHAPES] = {
	  {0, 0,         0, 0, TORSO_WIDTH, TORSO_LEN, TORSO_THICK, g_cube}, // torso
	  {1, ARM_LEN / 2, 0, 0, ARM_LEN / 2, ARM_THICK / 2, ARM_THICK / 2, g_sphere}, // upper right arm
	  {2, ARM_LEN / 2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower right arm
	  {3, -ARM_LEN / 2, 0, 0, ARM_LEN / 2, ARM_THICK / 2, ARM_THICK / 2, g_sphere}, // upper left arm
	  {4, -ARM_LEN / 2, 0, 0, ARM_LEN, ARM_THICK, ARM_THICK, g_cube}, // lower left arm
	  {5, 0, -ARM_LEN * 1.5 / 2, 0, ARM_THICK / 2, ARM_LEN * 1.5 / 2, ARM_THICK / 2, g_sphere}, // upper right leg
	  {6, 0, -ARM_LEN * 1.5 / 2, 0, ARM_THICK, ARM_LEN * 1.5, ARM_THICK, g_cube}, // lower right leg
	  {7, 0, -ARM_LEN * 1.5 / 2, 0, ARM_THICK / 2, ARM_LEN * 1.5 / 2, ARM_THICK / 2, g_sphere}, // upper left leg
	  {8, 0, -ARM_LEN * 1.5 / 2, 0, ARM_THICK, ARM_LEN * 1.5, ARM_THICK, g_cube}, // lower left leg
	  {9, 0, TORSO_WIDTH / 2, 0, TORSO_WIDTH / 3, TORSO_WIDTH / 3, TORSO_WIDTH / 3, g_sphere}, // head
	};

	shared_ptr<SgTransformNode> jointNodes[NUM_JOINTS];

	for (int i = 0; i < NUM_JOINTS; ++i) {
		if (jointDesc[i].parent == -1)
			jointNodes[i] = base;
		else {
			jointNodes[i].reset(new SgRbtNode(RigTForm(Cvec3(jointDesc[i].x, jointDesc[i].y, jointDesc[i].z))));
			jointNodes[jointDesc[i].parent]->addChild(jointNodes[i]);
		}
	}
	for (int i = 0; i < NUM_SHAPES; ++i) {
		shared_ptr<SgGeometryShapeNode> shape(
			new MyShapeNode(shapeDesc[i].geometry,
				material, // USE MATERIAL as opposed to color
				Cvec3(shapeDesc[i].x, shapeDesc[i].y, shapeDesc[i].z),
				Cvec3(0, 0, 0),
				Cvec3(shapeDesc[i].sx, shapeDesc[i].sy, shapeDesc[i].sz)));
		jointNodes[shapeDesc[i].parentJointId]->addChild(shape);
	}
}

static void initScene() {
	g_world.reset(new SgRootNode());

	g_skyNode.reset(new SgRbtNode(RigTForm(Cvec3(0.0, 0.25, 4.0))));

	g_groundNode.reset(new SgRbtNode());
	g_groundNode->addChild(shared_ptr<MyShapeNode>(
		new MyShapeNode(g_ground, g_bumpFloorMat, Cvec3(0, g_groundY, 0))));

	g_robot1Node.reset(new SgRbtNode(RigTForm(Cvec3(-2, 1, 0))));
	g_robot2Node.reset(new SgRbtNode(RigTForm(Cvec3(2, 1, 0))));

	constructRobot(g_robot1Node, g_redDiffuseMat); // a Red robot
	constructRobot(g_robot2Node, g_blueDiffuseMat); // a Blue robot

	g_light1Node.reset(new SgRbtNode(RigTForm(g_light1)));
	g_light2Node.reset(new SgRbtNode(RigTForm(g_light2)));
	g_light1Node->addChild(shared_ptr<MyShapeNode>(
		new MyShapeNode(g_sphere, g_lightMat, Cvec3(0, 0, 0), Cvec3(0, 0, 0), Cvec3(0.5, 0.5, 0.5))));
	g_light2Node->addChild(shared_ptr<MyShapeNode>(
		new MyShapeNode(g_sphere, g_lightMat, Cvec3(0, 0, 0), Cvec3(0, 0, 0), Cvec3(0.5, 0.5, 0.5))));

	g_world->addChild(g_skyNode);
	g_world->addChild(g_groundNode);
	g_world->addChild(g_robot1Node);
	g_world->addChild(g_robot2Node);
	g_world->addChild(g_light1Node);
	g_world->addChild(g_light2Node);
	dumpSgRbtNodes(g_world, curRbtnodes);
}

int main(int argc, char* argv[]) {
	try {
		initGlutState(argc, argv);

		glewInit(); // load the OpenGL extensions

		cout << (g_Gl2Compatible ? "Will use OpenGL 2.x / GLSL 1.0" : "Will use OpenGL 3.x / GLSL 1.3") << endl;
		if ((!g_Gl2Compatible) && !GLEW_VERSION_3_0)
			throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.3");
		else if (g_Gl2Compatible && !GLEW_VERSION_2_0)
			throw runtime_error("Error: card/driver does not support OpenGL Shading Language v1.0");

		initGLState();
		initMaterials();
		initGeometry();

		initScene();

		glutMainLoop();
		return 0;
	}
	catch (const runtime_error& e) {
		cout << "Exception caught: " << e.what() << endl;
		return -1;
	}
}
