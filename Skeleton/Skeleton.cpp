//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Hubert Annamaria
// Neptun : E3ONN7
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec4 vertexPosition;	// Attrib Array 0

	void main() {
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

const char * fragmentSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";

struct Camera {
	float wCx, wCy;	
	float wWx, wWy;	
public:
	Camera() {
		Animate(0);
	}

	mat4 V() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-wCx, -wCy, 0, 1);
	}

	mat4 P() {
		return mat4(2 / wWx, 0, 0, 0,
			0, 2 / wWy, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	mat4 Vinv() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			wCx, wCy, 0, 1);
	}

	mat4 Pinv() { 
		return mat4(wWx / 2, 0, 0, 0,
			0, wWy / 2, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	void Animate(float t) {
		wCx = 0; 
		wCy = 0;
		wWx = 20;
		wWy = 20;
	}
};

Camera camera;
const int nTesselatedVertices = 100;

vec4 calculatev(vec4 p0, vec4 p1, vec4 p2, float t0, float t1, float t2) {
	vec4 p21 = p2 - p1;
	vec4 p10 = p1 - p0;
	float t21 = t2 - t1;
	float t10 = t1 - t0;
	t21 = 1 / t21;
	t10 = 1 / t10;
	p21 = p21 * t21;
	p10 = p10 * t10;
	vec4 r = p21 + p10;
	r = r * 0.5;
	return r;
}

vec4 calculatea(vec4 p0, vec4 p1, vec4 v0, vec4 v1, float t0, float t1, float n) {
	vec4 p10, v10, r;
	p10 = p1 - p0;
	float cn, t10, tn, tn_1;
	cn = 5 - n;
	p10 = p10 * cn;
	cn = 4 - n;
	v0 = v0 * cn;
	t10 = t1 - t0;
	v10 = v1 + v0;
	tn = pow(t10, n);
	cn = n - 1;
	tn_1 = pow(t10, cn);
	tn = 1 / tn;
	tn_1 = 1 / tn_1;
	p10 = p10 * tn;
	v10 = v10 * tn_1;
	cn = n - 2.5;
	printf("cn %f\n", cn);
	cn *= 2;
	printf("cn %f\n", cn);
	v10 = v10 * cn;
	r = p10 + v10;

	return r;
}

GPUProgram gpuProgram;


class CatmullRom {

	unsigned int vaoCurve, vboCurve;
	unsigned int vaoCtrlPoints, vboCtrlPoints;

	std::vector<vec4>  cps; 
	std::vector<float> ts;	 
	std::vector<vec4>  points;

	vec4 Hermite(vec4 p0, vec4 v0, float t0, vec4 p1, vec4 v1, float t1, float t) {
		vec4 a0, a1, a2, a3;
		a0 = p0;
		a1 = v0;
		a2 = calculatea(p0, p1, v0, v1, t0, t1, 2);
		a3 = calculatea(p1, p0, v0, v1, t0, t1, 3);
		float tt0, tt03, tt02;
		tt0 = t - t0;
		tt03 = pow(tt0, 3);
		tt02 = pow(tt0, 2);
		a3 = a3 * tt03;
		a2 = a2 * tt02;
		a1 = a1 * tt0;
		return a3 + a2 + a1 + a0;
	}

	float L(int i, float t) {
		float Li = 1.0f;
		for (unsigned int j = 0; j < cps.size(); j++)
			if (j != i) Li *= (t - ts[j]) / (ts[i] - ts[j]);
		return Li;
	}

public:

	CatmullRom() {
		glGenVertexArrays(1, &vaoCurve);
		glBindVertexArray(vaoCurve);
		glGenBuffers(1, &vboCurve);
		glBindBuffer(GL_ARRAY_BUFFER, vboCurve);
		glEnableVertexAttribArray(0); 
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
	}

	float tStart() { return ts[0]; }
	float tEnd() { return ts[cps.size() - 1]; }

	void AddControlPoint(float cX, float cY) {
		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		cps.push_back(wVertex);
		float ti = cps.size(); 
		ts.push_back(ti);
	}

	vec4 r(float t) {
		for (int i = 0; i < cps.size() - 1; i++)
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec4 p0, v0, p1, v1, p_, p2;
				float t0, t1, t_, t2;
				p0 = cps[i];
				t0 = ts[i];
				p1 = cps[i + 1];
				t1 = ts[i + 1];
				if (i != 0) {
					p_ = cps[i - 1];
					t_ = ts[i - 1];
				}
				else {
					p_ = cps[0];
					t_ = 0;
				}
				if (i + 2 < cps.size()) {
					p2 = cps[i + 2];
					t2 = ts[i + 2];
				}
				else {
					p2 = cps[cps.size() - 1];
					t2 = 7;
				}
				v0 = calculatev(p_, p0, p1, t_, t0, t1);
				v1 = calculatev(p0, p1, p2, t0, t1, t2);
				return Hermite(p0, v0, t0, p1, v1, t1, t);
			}
	}

	void Draw() {

		mat4 VPTransform = camera.V() * camera.P();

		VPTransform.SetUniform(gpuProgram.getId(), "MVP");

		int colorLocation = glGetUniformLocation(gpuProgram.getId(), "color");

		if (cps.size() >= 2) {
			std::vector<float> vertexData;
			for (int i = 0; i < nTesselatedVertices; i++) {	
				float tNormalized = (float)i / (nTesselatedVertices - 1);
				float t = tStart() + (tEnd() - tStart()) * tNormalized;
				vec4 wVertex = r(t);
				vertexData.push_back(wVertex.x);
				vertexData.push_back(wVertex.y);
			}
			glBindVertexArray(vaoCurve);
			glBindBuffer(GL_ARRAY_BUFFER, vboCurve);
			glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);
			if (colorLocation >= 0) glUniform3f(colorLocation, 1, 1, 0);
			glDrawArrays(GL_LINE_STRIP, 0, nTesselatedVertices);
		}
	}
};

CatmullRom * crm;
CatmullRom * crp;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glLineWidth(2.0f);

	crm = new CatmullRom();
	crp = new CatmullRom();

	float cX = 2.0f * 0 / windowWidth - 1;
	float cY = 1.0f - 2.0f * 150 / windowHeight;
	crm->AddControlPoint(cX, cY);
	cX = 2.0f * 150 / windowWidth - 1;
	cY = 1.0f - 2.0f * 20 / windowHeight;
	crm->AddControlPoint(cX, cY);
	cX = 2.0f * 300 / windowWidth - 1;
	cY = 1.0f - 2.0f * 300 / windowHeight;
	crm->AddControlPoint(cX, cY);
	cX = 2.0f * 500 / windowWidth - 1;
	cY = 1.0f - 2.0f * 100 / windowHeight;
	crm->AddControlPoint(cX, cY);
	cX = 2.0f * 600 / windowWidth - 1;
	cY = 1.0f - 2.0f * 120 / windowHeight;
	crm->AddControlPoint(cX, cY);

	gpuProgram.Create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0, 0, 1, 0);							
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	crm->Draw();
	crp->Draw();

	glutSwapBuffers();						
}

void onKeyboard(unsigned char key, int pX, int pY) {   
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onMouseMotion(int pX, int pY) {
}

void onMouse(int button, int state, int pX, int pY) { 
	float cX = 2.0f * pX / windowWidth - 1;
	float cY = 1.0f - 2.0f * pY / windowHeight;
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  
		crp->AddControlPoint(cX, cY);
		glutPostRedisplay();  
	}
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); 
	glutPostRedisplay();
}