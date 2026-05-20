//	Biblioteci
#include <windows.h>        //	Utilizarea functiilor de sistem Windows (crearea de ferestre, manipularea fisierelor si directoarelor);
#include <stdlib.h>         //  Biblioteci necesare pentru citirea shaderelor;
#include <stdio.h>
#include <cstdlib> 
#include <vector>
#include <math.h>			//	Biblioteca pentru calcule matematice;
#include <GL/glew.h>        //  Defineste prototipurile functiilor OpenGL si constantele necesare pentru programarea OpenGL moderna; 
#include <GL/freeglut.h>    //	Include functii pentru: 
							//	- gestionarea ferestrelor si evenimentelor de tastatura si mouse, 
							//  - desenarea de primitive grafice precum dreptunghiuri, cercuri sau linii, 
							//  - crearea de meniuri si submeniuri;
#include "loadShaders.h"	//	Fisierul care face legatura intre program si shadere;
#include "glm/glm.hpp"		//	Bibloteci utilizate pentru transformari grafice;
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "SOIL.h"			//	Biblioteca pentru texturare;
#include "objloader.hpp" 


using namespace std;
float PI = 3.141592;

GLuint
VaoId, VaoId2,
VboId1, VboId2,
EboId1, EboId2,
ProgramId,
myMatrixLocation,
matrUmbraLocation,
viewLocation,
projLocation,
matrRotlLocation,
codColLocation,
lightColorLoc,
lightPosLoc,
viewPosLoc,
codCol,
sunColorLoc,
sunDirLoc,
lighSourceLocation,
objectTextureLocation,
useTextureLocation,
floorTextureId,
tableTextureId,
chandelierTextureId,
couchTextureId,
armchairTextureId,
tvScreenTextureId,
materialSpecularStrengthLoc,
materialShininessLoc,
VaoIdObj1, VboIdObj1,
VaoIdObj2, VboIdObj2,
VaoIdCone, VboIdCone,
VaoIdObj3, VboIdObj3,
VaoIdObj4, VboIdObj4,
VaoIdObj5, VboIdObj5,
VaoIdObjTvScreen, VboIdObjTvScreen,
VaoIdObjTvBody, VboIdObjTvBody;

// elemente pentru matricea de vizualizare si matricea de proiectie
// Pozitia camerei (initial in centrul camerei, putin deasupra podelei)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 150.0f);
// Directia in care se uita camera
glm::vec3 cameraFront = glm::vec3(-1.0f, 0.0f, 0.0f);
// Vectorul "Sus"
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

// Unghiurile de rotatie (Yaw - stanga/dreapta, Pitch - sus/jos)
float yaw = 180.0f; 
float pitch = 0.0f;
float cameraSpeed = 5.0f; // Viteza de deplasare
float width = 1200, height = 900, znear = 0.1, zfar = 5000.0f, fov = 70;


// Variabila pentru starea luminii
// 0 = Interior, 1 = Exterior, 2 = Ambele
GLuint lightSourceType;

// matrice
glm::mat4 view, projection, matrUmbra, myMatrix;

// Vectori pentru modelul OBJ
std::vector<glm::vec3> obj_vertices1, obj_vertices2, obj_vertices3, obj_vertices4, obj_vertices5, 
						obj_verticesTvScreen, obj_verticesTvBody;
std::vector<glm::vec2> obj_uvs1, obj_uvs2, obj_uvs3, obj_uvs4, obj_uvs5, obj_uvsTvScreen, 
						obj_uvsTvBody;
std::vector<glm::vec3> obj_normals1, obj_normals2, obj_normals3, obj_normals4, obj_normals5, 
						obj_normalsTvScreen, obj_normalsTvBody;

int nrVerticesObj1, nrVerticesObj2, nrVerticesCone, nrVerticesObj3, nrVerticesObj4, nrVerticesObj5, 
	nrVerticesObjTvScreen, nrVerticesObjTvBody;


//Variabile Shadow mapping
GLuint depthFBO = 0, depthTex = 0, depthProg = 0;
const int SHW = 1024, SHH = 1024;
glm::mat4 lightSpace;

glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, -0.1f, -0.3f));

void processNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
	case 'w': case 'W': cameraPos += cameraSpeed * cameraFront; break;
	case 's': case 'S': cameraPos -= cameraSpeed * cameraFront; break;
	case 'a': case 'A': cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; break;
	case 'd': case 'D': cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; break;
	case 'l': case 'L':
		lightSourceType = (lightSourceType + 1) % 2;
		glUniform1i(lighSourceLocation, lightSourceType);
		break;
	}

	if (key == 27)
		exit(0);

	// Constrangere: Ramanem in interiorul camerei (podeaua e între -500 și 500 pe X, -400 și 400 pe Y)
	cameraPos.x = glm::clamp(cameraPos.x, -480.0f, 480.0f);
	cameraPos.y = glm::clamp(cameraPos.y, -380.0f, 380.0f);
	cameraPos.z = glm::clamp(cameraPos.z, 20.0f, 480.0f); // Sa nu trecem prin tavan/podea

	glutPostRedisplay();
}
void processSpecialKeys(int key, int xx, int yy) {
	float rotSpeed = 1.0f;
	switch (key) {
	case GLUT_KEY_LEFT:  yaw -= rotSpeed; break;
	case GLUT_KEY_RIGHT: yaw += rotSpeed; break;
	case GLUT_KEY_UP:    pitch += rotSpeed; break;
	case GLUT_KEY_DOWN:  pitch -= rotSpeed; break;
	}


	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.z = sin(glm::radians(pitch));
	cameraFront = glm::normalize(direction);

	glutPostRedisplay();
}

//	Functia de incarcare a texturilor in program;
GLuint LoadTexture(const char* texturePath)
{
	GLuint texId;

	// Generarea unui obiect textura si legarea acestuia;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);

	//	Desfasurarea imaginii pe orizontala/verticala in functie de parametrii de texturare;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Modul in care structura de texeli este aplicata pe cea de pixeli;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Incarcarea texturii si transferul datelor in obiectul textura; 
	int width, height;
	unsigned char* image = SOIL_load_image(texturePath, &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	// Eliberarea resurselor
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texId;
}
void CreateVBO(void)
{
	// Coordonate: x, y, z, w, r, g, b, a, nx, ny, nz
	// Normalele trebuie sa arate spre interiorul camerei pentru a fi luminate corect din interior

	GLfloat VerticesRoom[] = {
		// Coordonate: x, y, z, w,  r, g, b, a,  nx, ny, nz

		// --- PODEA (Z=0, Normala: SUS) ---
		-500.0f, -400.0f,  0.0f, 1.0f,  0.6f, 0.4f, 0.2f, 1.0f,  0.0f, 0.0f, 1.0f,
		 500.0f, -400.0f,  0.0f, 1.0f,  0.6f, 0.4f, 0.2f, 1.0f,  0.0f, 0.0f, 1.0f,
		 500.0f,  400.0f,  0.0f, 1.0f,  0.6f, 0.4f, 0.2f, 1.0f,  0.0f, 0.0f, 1.0f,
		-500.0f,  400.0f,  0.0f, 1.0f,  0.6f, 0.4f, 0.2f, 1.0f,  0.0f, 0.0f, 1.0f,

		// --- TAVAN (Z=500, Normala: JOS) ---
		-500.0f, -400.0f, 500.0f, 1.0f,  0.9f, 0.9f, 0.9f, 1.0f,  0.0f, 0.0f, -1.0f,
		 500.0f, -400.0f, 500.0f, 1.0f,  0.9f, 0.9f, 0.9f, 1.0f,  0.0f, 0.0f, -1.0f,
		 500.0f,  400.0f, 500.0f, 1.0f,  0.9f, 0.9f, 0.9f, 1.0f,  0.0f, 0.0f, -1.0f,
		-500.0f,  400.0f, 500.0f, 1.0f,  0.9f, 0.9f, 0.9f, 1.0f,  0.0f, 0.0f, -1.0f,

		// --- PERETE STANGA (Y=-400, Normala: spre centrul Y+) ---
		-500.0f, -400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, 1.0f, 0.0f,
		 500.0f, -400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, 1.0f, 0.0f,
		 500.0f, -400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, 1.0f, 0.0f,
		-500.0f, -400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, 1.0f, 0.0f,

		// --- PERETE DREAPTA (Y=400, Normala: spre centrul Y-) ---
		-500.0f,  400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, -1.0f, 0.0f,
		 500.0f,  400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, -1.0f, 0.0f,
		 500.0f,  400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, -1.0f, 0.0f,
		-500.0f,  400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  0.0f, -1.0f, 0.0f,

		// --- PERETE FATA CU FEREASTRA (X=-500, Normala: spre centrul X+) ---
		// Toate segmentele acestui perete au aceeasi normala (1, 0, 0)

		// 1. Bucata de jos
		-500.0f, -400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f, -400.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,

		// 2. Bucata de sus
		-500.0f, -400.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f, -400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,

		// 3. Bucata stanga fereastra
		-500.0f, -400.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f, -200.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f, -200.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f, -400.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,

		// 4. Bucata dreapta fereastra
		-500.0f,  200.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f, 100.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  400.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,
		-500.0f,  200.0f, 400.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f,  1.0f, 0.0f, 0.0f,

		// --- PERETE SPATE (X=500, Normala: spre centrul X-) ---
		 500.0f, -400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f, -1.0f, 0.0f, 0.0f,
		 500.0f,  400.0f,   0.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f, -1.0f, 0.0f, 0.0f,
		 500.0f,  400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f, -1.0f, 0.0f, 0.0f,
		 500.0f, -400.0f, 500.0f, 1.0f,  0.87f, 0.81f, 0.75f, 1.0f, -1.0f, 0.0f, 0.0f
	};

	// Indicii (cate 2 triunghiuri per dreptunghi)
	// Avem 8 dreptunghiuri in total (Podea, Tavan, Spate, Dreapta, StangaJos, StangaSus, StangaFata, StangaSpate)
	GLubyte IndicesRoom[] = {
	0, 1, 2, 2, 3, 0,       // Podea
	4, 5, 6, 6, 7, 4,       // Tavan
	8, 9, 10, 10, 11, 8,    // Perete lateral 1
	12, 13, 14, 14, 15, 12, // Perete lateral 2
	16, 17, 18, 18, 19, 16, // Fereastra segment 1
	20, 21, 22, 22, 23, 20, // Fereastra segment 2
	24, 25, 26, 26, 27, 24, // Fereastra segment 3
	28, 29, 30, 30, 31, 28, // Fereastra segment 4
	32, 33, 34, 34, 35, 32  // ACUM VA APAREA PERETELE SPATE
	};

	glGenVertexArrays(1, &VaoId);
	glBindVertexArray(VaoId);
	glGenBuffers(1, &VboId1);
	glGenBuffers(1, &EboId1);

	glBindBuffer(GL_ARRAY_BUFFER, VboId1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VerticesRoom), VerticesRoom, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndicesRoom), IndicesRoom, GL_STATIC_DRAW);

	// Activare atribute (pozitie, culoare, normala)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));

	glBindVertexArray(0);
	

	// Podea - VAO separat (cu UV)

	// x,y,z,w  r,g,b,a  nx,ny,nz  u,v  -> 13 floats
	GLfloat VerticesFloor[] = {
		-500.0f, -400.0f, 0.0f, 1.0f,   1.0f,1.0f,1.0f,1.0f,   0.0f,0.0f,1.0f,   0.0f, 0.0f,
		 500.0f, -400.0f, 0.0f, 1.0f,   1.0f,1.0f,1.0f,1.0f,   0.0f,0.0f,1.0f,   3.0f, 0.0f,
		 500.0f,  400.0f, 0.0f, 1.0f,   1.0f,1.0f,1.0f,1.0f,   0.0f,0.0f,1.0f,   3.0f, 3.0f,
		-500.0f,  400.0f, 0.0f, 1.0f,   1.0f,1.0f,1.0f,1.0f,   0.0f,0.0f,1.0f,   0.0f, 3.0f
	};

	GLubyte IndicesFloor[] = { 0,1,2, 2,3,0 };

	glGenVertexArrays(1, &VaoId2);
	glBindVertexArray(VaoId2);

	glGenBuffers(1, &VboId2);
	glGenBuffers(1, &EboId2);

	glBindBuffer(GL_ARRAY_BUFFER, VboId2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VerticesFloor), VerticesFloor, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndicesFloor), IndicesFloor, GL_STATIC_DRAW);

	GLsizei stride = 13 * sizeof(GLfloat);

	// position (loc 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);

	// color (loc 1)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(4 * sizeof(GLfloat)));

	// normal (loc 2)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat)));

	// texcoord (loc 3)
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(11 * sizeof(GLfloat)));

	glBindVertexArray(0);


	// Configurare Pentru Modelul OBJ 1
	if (nrVerticesObj1 > 0) {
		glGenVertexArrays(1, &VaoIdObj1);
		glBindVertexArray(VaoIdObj1);

		glGenBuffers(1, &VboIdObj1);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObj1);

		// dimensiunile pentru BufferData
		int sizeV = obj_vertices1.size() * sizeof(glm::vec3);
		int sizeN = obj_normals1.size() * sizeof(glm::vec3);
		int sizeU = obj_uvs1.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_vertices1[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normals1[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvs1[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Modelul OBJ 2
	if (nrVerticesObj2 > 0) {
		glGenVertexArrays(1, &VaoIdObj2);
		glBindVertexArray(VaoIdObj2);

		glGenBuffers(1, &VboIdObj2);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObj2);

		// dimensiunile pentru BufferData
		int sizeV = obj_vertices2.size() * sizeof(glm::vec3);
		int sizeN = obj_normals2.size() * sizeof(glm::vec3);
		int sizeU = obj_uvs2.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_vertices2[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normals2[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvs2[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Modelul OBJ 3
	if (nrVerticesObj3 > 0) {
		glGenVertexArrays(1, &VaoIdObj3);
		glBindVertexArray(VaoIdObj3);

		glGenBuffers(1, &VboIdObj3);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObj3);

		// dimensiunile pentru BufferData
		int sizeV = obj_vertices3.size() * sizeof(glm::vec3);
		int sizeN = obj_normals3.size() * sizeof(glm::vec3);
		int sizeU = obj_uvs3.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_vertices3[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normals3[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvs3[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Modelul OBJ 4
	if (nrVerticesObj4 > 0) {
		glGenVertexArrays(1, &VaoIdObj4);
		glBindVertexArray(VaoIdObj4);

		glGenBuffers(1, &VboIdObj4);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObj4);

		// dimensiunile pentru BufferData
		int sizeV = obj_vertices4.size() * sizeof(glm::vec3);
		int sizeN = obj_normals4.size() * sizeof(glm::vec3);
		int sizeU = obj_uvs4.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_vertices4[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normals4[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvs4[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Modelul OBJ 5
	if (nrVerticesObj5 > 0) {
		glGenVertexArrays(1, &VaoIdObj5);
		glBindVertexArray(VaoIdObj5);

		glGenBuffers(1, &VboIdObj5);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObj5);

		// dimensiunile pentru BufferData
		int sizeV = obj_vertices5.size() * sizeof(glm::vec3);
		int sizeN = obj_normals5.size() * sizeof(glm::vec3);
		int sizeU = obj_uvs5.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_vertices5[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normals5[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvs5[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Tv Screen
	if (nrVerticesObjTvScreen > 0) {
		glGenVertexArrays(1, &VaoIdObjTvScreen);
		glBindVertexArray(VaoIdObjTvScreen);

		glGenBuffers(1, &VboIdObjTvScreen);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObjTvScreen);

		// dimensiunile pentru BufferData
		int sizeV = obj_verticesTvScreen.size() * sizeof(glm::vec3);
		int sizeN = obj_normalsTvScreen.size() * sizeof(glm::vec3);
		int sizeU = obj_uvsTvScreen.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_verticesTvScreen[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normalsTvScreen[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvsTvScreen[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}

	// Configurare Pentru Tv Body
	if (nrVerticesObjTvBody > 0) {
		glGenVertexArrays(1, &VaoIdObjTvBody);
		glBindVertexArray(VaoIdObjTvBody);

		glGenBuffers(1, &VboIdObjTvBody);
		glBindBuffer(GL_ARRAY_BUFFER, VboIdObjTvBody);

		// dimensiunile pentru BufferData
		int sizeV = obj_verticesTvBody.size() * sizeof(glm::vec3);
		int sizeN = obj_normalsTvBody.size() * sizeof(glm::vec3);
		int sizeU = obj_uvsTvBody.size() * sizeof(glm::vec2);

		// alocare spatiu si incarcare date secvential
		glBufferData(GL_ARRAY_BUFFER, sizeV + sizeN + sizeU, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeV, &obj_verticesTvBody[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV, sizeN, &obj_normalsTvBody[0]);
		glBufferSubData(GL_ARRAY_BUFFER, sizeV + sizeN, sizeU, &obj_uvsTvBody[0]);

		// Atribut 0: Pozitie (in OBJ e vec3, shaderul vrea vec4)
		// OpenGL pune automat w = 1.0 dacz trimit 3 componente
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Atribut 1: Culoare (OBJ nu are culori per vertex)
		// Dezactivez array-ul pentru locatia 1 si setezm o culoare generica (ex: maro pentru masa)
		// In shader nu se va uita dupa culoare in VBO pentru fiecare vertex,
		// ci se va folosi o culoare statica
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.0f, 0.0f, 0.0f, 1.0f);

		// Atribut 2: Normale (vec3)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeV);

		// Atribut 3: Coordonate texturare (vec2)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeV + sizeN));

		glBindVertexArray(0);
	}
}

void CreateCone(float radius, float height, int segments) {
	std::vector<GLfloat> coneVertices;

	// Varful conului (Apex)
	// x, y, z, w,  r, g, b, a,  nx, ny, nz,  u, v
	glm::vec3 apexPos(0.0f, 0.0f, height);

	for (int i = 0; i <= segments; i++) {
		float angle = 2.0f * PI * float(i) / float(segments);
		float nextAngle = 2.0f * PI * float(i + 1) / float(segments);

		// Punctele de pe cerc (baza)
		float x1 = radius * cos(angle);
		float y1 = radius * sin(angle);
		float x2 = radius * cos(nextAngle);
		float y2 = radius * sin(nextAngle);

		// Calcul normale 
		glm::vec3 p1(x1, y1, 0.0f);
		glm::vec3 p2(x2, y2, 0.0f);
		glm::vec3 normal = glm::normalize(glm::cross(p2 - apexPos, p1 - apexPos));

		// Apex
		coneVertices.insert(coneVertices.end(), { 0.0f, 0.0f, height, 1.0f,   0.7f, 0.7f, 0.7f, 1.0f,  normal.x, normal.y, normal.z, 0.5f, 1.0f });
		// Punct 1
		coneVertices.insert(coneVertices.end(), { x1, y1, 0.0f, 1.0f,   0.7f, 0.7f, 0.7f, 1.0f,  normal.x, normal.y, normal.z, float(i) / segments, 0.0f });
		// Punct 2
		coneVertices.insert(coneVertices.end(), { x2, y2, 0.0f, 1.0f,   0.7f, 0.7f, 0.7f, 1.0f,  normal.x, normal.y, normal.z, float(i + 1) / segments, 0.0f });
	}

	nrVerticesCone = coneVertices.size() / 13;

	glGenVertexArrays(1, &VaoIdCone);
	glBindVertexArray(VaoIdCone);
	glGenBuffers(1, &VboIdCone);
	glBindBuffer(GL_ARRAY_BUFFER, VboIdCone);
	glBufferData(GL_ARRAY_BUFFER, coneVertices.size() * sizeof(GLfloat), coneVertices.data(), GL_STATIC_DRAW);

	GLsizei stride = 13 * sizeof(GLfloat);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(4 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(11 * sizeof(GLfloat)));

	glBindVertexArray(0);
}
/*void AssociateAttributePointers()
{
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
}*/
void DestroyVBO(void)
{
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDeleteBuffers(1, &VboId1);
	glDeleteBuffers(1, &EboId1);
	glDeleteVertexArrays(1, &VaoId);

	glDeleteBuffers(1, &VboId2);
	glDeleteBuffers(1, &EboId2);
	glDeleteVertexArrays(1, &VaoId2);

	glDeleteBuffers(1, &VboIdObj1);
    glDeleteVertexArrays(1, &VaoIdObj1);

	glDeleteBuffers(1, &VboIdObj2);
	glDeleteVertexArrays(1, &VaoIdObj2);

	glDeleteBuffers(1, &VboIdCone);
	glDeleteVertexArrays(1, &VaoIdCone);

	glDeleteBuffers(1, &VboIdObj3);
	glDeleteVertexArrays(1, &VaoIdObj3);

	glDeleteBuffers(1, &VboIdObj4);
	glDeleteVertexArrays(1, &VaoIdObj4);

	glDeleteBuffers(1, &VboIdObj5);
	glDeleteVertexArrays(1, &VaoIdObj5);

	glDeleteBuffers(1, &VboIdObjTvScreen);
	glDeleteVertexArrays(1, &VaoIdObjTvScreen);


	glDeleteBuffers(1, &VboIdObjTvBody);
	glDeleteVertexArrays(1, &VaoIdObjTvBody);
}
void CreateShaders(void)
{
	ProgramId = LoadShaders("Shader.vert", "Shader.frag");
	glUseProgram(ProgramId);
}
void DestroyShaders(void)
{
	glDeleteProgram(ProgramId);
}
void Initialize(void)
{
	glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // culoarea de fond a ecranului
	CreateShaders();

	// Incarcare model 1
	bool modelLoaded = loadOBJ("Table2.obj", obj_vertices1, obj_uvs1, obj_normals1);
	if (modelLoaded) {
		nrVerticesObj1 = obj_vertices1.size();
	}

	// Incarcare model 2
	modelLoaded = loadOBJ("lightbulb2.obj", obj_vertices2, obj_uvs2, obj_normals2);
	if (modelLoaded) {
		nrVerticesObj2 = obj_vertices2.size();
	}

	// Incarcare model 3
	modelLoaded = loadOBJ("couch2.obj", obj_vertices3, obj_uvs3, obj_normals3);
	if (modelLoaded) {
		nrVerticesObj3 = obj_vertices3.size();
	}

	// Incarcare model 4
	modelLoaded = loadOBJ("Armchair2.obj", obj_vertices4, obj_uvs4, obj_normals4);
	if (modelLoaded) {
		nrVerticesObj4 = obj_vertices4.size();
	}

	// Incarcare model 5
	modelLoaded = loadOBJ("vase3_2.obj", obj_vertices5, obj_uvs5, obj_normals5);
	if (modelLoaded) {
		nrVerticesObj5 = obj_vertices5.size();
	}

	// Incarcare model Tv Screen
	modelLoaded = loadOBJ("tv_screen.obj", obj_verticesTvScreen, obj_uvsTvScreen, obj_normalsTvScreen);
	if (modelLoaded) {
		nrVerticesObjTvScreen = obj_verticesTvScreen.size();
	}

	// Incarcare model Tv Body
	modelLoaded = loadOBJ("tv_body2.obj", obj_verticesTvBody, obj_uvsTvBody, obj_normalsTvBody);
	if (modelLoaded) {
		nrVerticesObjTvBody = obj_verticesTvBody.size();
	}

	CreateVBO();
	CreateCone(18.0f, 37.0f, 60);

	viewLocation = glGetUniformLocation(ProgramId, "view");
	projLocation = glGetUniformLocation(ProgramId, "projection");
	myMatrixLocation = glGetUniformLocation(ProgramId, "myMatrix");
	matrUmbraLocation = glGetUniformLocation(ProgramId, "matrUmbra");
	
	lightColorLoc = glGetUniformLocation(ProgramId, "lightColor");
	lightPosLoc = glGetUniformLocation(ProgramId, "lightPos");
	viewPosLoc = glGetUniformLocation(ProgramId, "viewPos");
	
	codColLocation = glGetUniformLocation(ProgramId, "codCol");
	lighSourceLocation = glGetUniformLocation(ProgramId, "lightSource");

	sunColorLoc = glGetUniformLocation(ProgramId, "sunColor");
	sunDirLoc = glGetUniformLocation(ProgramId, "sunDir");

	materialSpecularStrengthLoc = glGetUniformLocation(ProgramId, "materialSpecularStrength");
	materialShininessLoc = glGetUniformLocation(ProgramId, "materialShininess");

	useTextureLocation = glGetUniformLocation(ProgramId, "useTexture");
	objectTextureLocation = glGetUniformLocation(ProgramId, "objectTexture");

	floorTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Textures\\wooden_floor.png");
	tableTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Models\\coffee-table\\textures\\None_albedo.jpeg");
	chandelierTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Textures\\lamp_cover.jpg");
	couchTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Textures\\sofa_fabric.jpg");
	armchairTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Textures\\armchair_texture.jpg");
	tvScreenTextureId = LoadTexture("D:\\Facultate\\An4_sem1\\Grafica\\Proiect 2\\Models\\wall-flat-tv\\source\\wall-flat-tv-obj\\tv_screen.jpg");

	glUseProgram(ProgramId);
	glUniform1i(objectTextureLocation, 0);

	lightSourceType = 0; //initial lumina interioara activa(noapte)
	glUniform1i(lighSourceLocation, lightSourceType);

	depthProg = LoadShaders("Depth.vert", "Depth.frag");

	glGenFramebuffers(1, &depthFBO);
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHW, SHH, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float bc[4] = { 1,1,1,1 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);

	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void RenderDepthPass()
{
	// soarele
	/*glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, -0.1f, -0.5f));*/ //E globala
	glm::vec3 lightPos = -sunDir * 800.0f;

	glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0, 0, 200), glm::vec3(0, 0, 1));
	glm::mat4 lightProj = glm::ortho(-900.f, 900.f, -900.f, 900.f, 1.f, 2500.f);
	lightSpace = lightProj * lightView;

	glViewport(0, 0, SHW, SHH);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(depthProg);
	GLint uLS = glGetUniformLocation(depthProg, "lightSpaceMatrix");
	GLint uM = glGetUniformLocation(depthProg, "model");
	glUniformMatrix4fv(uLS, 1, GL_FALSE, glm::value_ptr(lightSpace));

	auto drawElems = [&](GLuint vao, int count, const glm::mat4& model) {
		glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_BYTE, 0);
		};
	auto drawArr = [&](GLuint vao, int count, const glm::mat4& model) {
		glUniformMatrix4fv(uM, 1, GL_FALSE, glm::value_ptr(model));
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, count);
		};

	// PODEA
	drawElems(VaoId2, 6, glm::mat4(1.0f));

	// CAMERA 
	drawElems(VaoId, 54, glm::mat4(1.0f));

	// MASA 
	if (nrVerticesObj1 > 0) {
		glm::mat4 modelTable =
			glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 27)) *
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1, 0, 0)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(60));
		drawArr(VaoIdObj1, nrVerticesObj1, modelTable);
	}

	if (nrVerticesObj5 > 0) {
		glm::mat4 modelVase = 
			glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 55.0f)) * //fix pe masa
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(6.5f, 6.5f, 6.5f));
		drawArr(VaoIdObj5, nrVerticesObj5, modelVase);
	}

	if (nrVerticesObj4 > 0) {
		glm::mat4 modelArmchair = glm::translate(glm::mat4(1.0f), glm::vec3(380.0f, -250.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), -3 * PI / 4, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(18.0f, 18.0f, 18.0f));
		drawArr(VaoIdObj4, nrVerticesObj4, modelArmchair);
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (int)width, (int)height);
}


void RenderFunction(void)
{
	RenderDepthPass();

	glUseProgram(ProgramId);
	glUniformMatrix4fv(glGetUniformLocation(ProgramId, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpace));
	glUniform1i(glGetUniformLocation(ProgramId, "shadowMap"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthTex);

	if (lightSourceType == 0) // Doar lumina interioara activa, noapte
		glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Albastru inchis
	else // Lumina exterioara activa, zi
		glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Albastru mai deschis
	glUseProgram(ProgramId);
	glBindVertexArray(VaoId);

	myMatrix = glm::mat4(1.0f);
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Matricea de vizualizare (LookAt)
	// Ne uitam de la cameraPos catre cameraPos + cameraFront (punctul din fata noastra)
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	// Transmitem pozitia camerei la shader pentru calculul specular / reflexii
	glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

	//Matricea de proiectie/perspectiva
	projection = glm::perspective(glm::radians(fov), GLfloat(width) / GLfloat(height), znear, zfar);
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);


	// UMBRA
	// Coordonatele becului (trebuie sa coincida cu lightPos din shader)
	float xL = 0.0f, yL = 0.0f, zL = 475.0f;
	float D = -0.5f; // Inaltimea la care "aterizeaza" umbra (putin deasupra podelei)

	float matrUmbra[4][4];

	matrUmbra[0][0] = zL + D;   matrUmbra[0][1] = 0;         matrUmbra[0][2] = 0;   matrUmbra[0][3] = 0;
	matrUmbra[1][0] = 0;        matrUmbra[1][1] = zL + D;    matrUmbra[1][2] = 0;   matrUmbra[1][3] = 0;
	matrUmbra[2][0] = -xL;      matrUmbra[2][1] = -yL;       matrUmbra[2][2] = D;   matrUmbra[2][3] = -1;
	matrUmbra[3][0] = -D * xL;  matrUmbra[3][1] = -D * yL;   matrUmbra[3][2] = -D * zL; matrUmbra[3][3] = zL;

	// Trimitem matricea la shader
	glUniformMatrix4fv(matrUmbraLocation, 1, GL_FALSE, &matrUmbra[0][0]);

	// ILUMINARE
	
	// 1. Lumina punctiforma (Lustra) - o punem sus, in mijloc
	glUniform3f(lightColorLoc, 1.0f, 0.95f, 0.89f); // Warm Fluorescent 255, 244, 229
	glUniform3f(lightPosLoc, 0.0f, 0.0f, 475.0f);  // In mijlocul camerei, sub tavan

	// 2. Lumina de Exterior (Directionala)
	glUniform3f(sunColorLoc, 0.96f, 0.8f, 0.7f);    // Clear Blue Sky 64, 156, 255
	glUniform3f(sunDirLoc, sunDir.x, sunDir.y, sunDir.z); // Directia din care bate soarele pe cer


	// DESENEAZA PODEA TEXTURATA	
	codCol = 0; // Iluminare activata
	glUniform1i(codColLocation, codCol);
	
	glUniform1i(useTextureLocation, 1); //folosirea texturii

	glUniform1f(materialSpecularStrengthLoc, 0.05f); //fara luciu pe podea sau foarte mic
	glUniform1f(materialShininessLoc, 8.0f); // highlight larg, difuz

	glActiveTexture(GL_TEXTURE0); //incarcarea texturii in texture unit 0
	glBindTexture(GL_TEXTURE_2D, floorTextureId);

	glBindVertexArray(VaoId2);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0); //golirea texture unit 0
	glBindVertexArray(0);

	
	
	// DESENARE SUFRAGERIE
	glUniform1i(useTextureLocation, 0);   // camera NU e texturata

	glUniform1f(materialSpecularStrengthLoc, 0.01f);
	glUniform1f(materialShininessLoc, 4.0f);

	glBindVertexArray(VaoId);
	// 8 dreptunghiuri * 2 triunghiuri * 3 varfuri = 48 indici
	glDrawElements(GL_TRIANGLES, 54 - 6, GL_UNSIGNED_BYTE, (GLvoid*)(6 * sizeof(GLubyte)));
	glBindVertexArray(0);
	
	// DESENARE MODEL OBJ 1 (MASA)
	if (nrVerticesObj1 > 0) {
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 27.0f)) *
				   glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
				   glm::scale(glm::mat4(1.0f), glm::vec3(60.0f, 60.0f, 60.0f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(codColLocation, 0);
		glUniform1i(useTextureLocation, 1);     
		glUniform1f(materialSpecularStrengthLoc, 0.3f);
		glUniform1f(materialShininessLoc, 32.0f);

		// Bind-uim textura mesei in unitatea 0 de texturare
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tableTextureId);

		// Activare VAO
		glBindVertexArray(VaoIdObj1);
		// Dezactivam citirea culorii din VBO (care oricum nu exista in OBJ)
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.5f, 0.35f, 0.05f, 1.0f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj1);

		// Reactivare atributul 1 pentru restul obiectelor din scena
		// Eliminam tectura mesei din unitatea de texturare
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		//UMBRA
		if (lightSourceType == 0) {
			glUniform1i(codColLocation, 1);

			// Activam blending-ul pentru o umbra mai realista (transparenta)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			//  Desenam din nou obiectul (Shaderul va aplica matrUmbra automat)
			glBindVertexArray(VaoIdObj1);
			glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj1);

			glDisable(GL_BLEND);
			glBindVertexArray(0);
		}
	}

	// DESENARE MODEL OBJ 3 (Canapea)
	if (nrVerticesObj3 > 0) {
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 300.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(25.0f, 25.0f, 25.0f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(codColLocation, 0);
		glUniform1i(useTextureLocation, 1);
		glUniform1f(materialSpecularStrengthLoc, 0.3f);
		glUniform1f(materialShininessLoc, 4.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, couchTextureId);

		// Activare VAO
		glBindVertexArray(VaoIdObj3);
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.5f, 0.35f, 0.05f, 1.0f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj3);

	
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		//UMBRA
		if (lightSourceType == 0) {
			glUniform1i(codColLocation, 1);

			// Activam blending-ul pentru o umbra mai realista (transparenta)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Desenam din nou obiectul (Shaderul va aplica matrUmbra automat)
			glBindVertexArray(VaoIdObj3);
			glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj3);

			glDisable(GL_BLEND);
			glBindVertexArray(0);
		}
	}

	// DESENARE MODEL OBJ 4 (Fotoliu)
	if (nrVerticesObj4 > 0) {
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(380.0f, 250.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), -PI / 4, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(18.0f, 18.0f, 18.0f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(codColLocation, 0);
		glUniform1i(useTextureLocation, 0);
		glUniform1f(materialSpecularStrengthLoc, 0.3f);
		glUniform1f(materialShininessLoc, 4.0f);

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, armchairTextureId);

		// Activare VAO
		glBindVertexArray(VaoIdObj4);
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 1.0f, 0.1f, 0.05f, 1.0f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj4);


		//glBindTexture(GL_TEXTURE_2D, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		//UMBRA
		if (lightSourceType == 0) {
			glUniform1i(codColLocation, 1);

			// Activam blending-ul pentru o umbra mai realista (transparenta)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Desenam din nou obiectul (Shaderul va aplica matrUmbra automat)
			glBindVertexArray(VaoIdObj4);
			glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj4);

			glDisable(GL_BLEND);
			glBindVertexArray(0);
		}
		//DESENARE AL 2-LEA FOTOLIU
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(380.0f, -250.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::rotate(glm::mat4(1.0f), -3 *PI / 4, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(18.0f, 18.0f, 18.0f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(codColLocation, 0);
		glUniform1i(useTextureLocation, 0);
		glUniform1f(materialSpecularStrengthLoc, 0.3f);
		glUniform1f(materialShininessLoc, 4.0f);

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, armchairTextureId);

		// Activare VAO
		glBindVertexArray(VaoIdObj4);
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 0.8f, 0.1f, 0.05f, 1.0f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj4);


		//glBindTexture(GL_TEXTURE_2D, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		//UMBRA
		if (lightSourceType == 0) {
			glUniform1i(codColLocation, 1);

			// Activam blending-ul pentru o umbra mai realista (transparenta)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Desenam din nou obiectul (Shaderul va aplica matrUmbra automat)
			glBindVertexArray(VaoIdObj4);
			glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj4);

			glDisable(GL_BLEND);
			glEnableVertexAttribArray(1);
			glBindVertexArray(0);
		}
	}

	//----------------------------------------------------------

	// --- 1. DESENEAZA CORPUL TV ---
	glm::mat4 modelTv = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -380.0f, 250.0f)) *
		glm::rotate(glm::mat4(1.0f), -PI/2, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), PI, glm::vec3(0.0f, 0.0f, 1.0f)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelTv));
	glUniform1i(codColLocation, 0);
	glUniform1i(useTextureLocation, 0); // Fara textura
	glUniform1f(materialSpecularStrengthLoc, 0.9f);
	glUniform1f(materialShininessLoc, 128.0f);

	glDisableVertexAttribArray(1);
	glVertexAttrib4f(1, 0.1f, 0.1f, 0.1f, 0.1f); // Negru spre gri inchis

	glBindVertexArray(VaoIdObjTvBody);
	glDrawArrays(GL_TRIANGLES, 0, nrVerticesObjTvBody);

	// --- 2. DESENEAZA ECRANUL ---
	glm::mat4 modelEcran = glm::translate(modelTv, glm::vec3(0.0f, 0.0f, 0.5f));
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelEcran));

	glUniform1i(useTextureLocation, 1); // Activam textura pentru imaginea TV
	glUniform1i(codColLocation, 0);
	glUniform1f(materialSpecularStrengthLoc, 0.9f);
	glUniform1f(materialShininessLoc, 128.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tvScreenTextureId);

	glBindVertexArray(VaoIdObjTvScreen);
	glDrawArrays(GL_TRIANGLES, 0, nrVerticesObjTvScreen);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	
	//----------------------------------------------------------------------

	// DESENARE LUSTRA (CON)
	myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 485.0f)) *
			glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)); 
	glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

	glUniform1i(useTextureLocation, 1); // Putem folosi o culoare solida sau o textura metalica
	glUniform1i(codColLocation, 0);
	glUniform1f(materialSpecularStrengthLoc, 0.8f);
	glUniform1f(materialShininessLoc, 64.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, chandelierTextureId);

	glBindVertexArray(VaoIdCone);
	glDrawArrays(GL_TRIANGLES, 0, nrVerticesCone);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);


	// DESENARE MODEL OBJ 2 (Bec)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (nrVerticesObj2 > 0) {
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 500.0f)) * //fix pe tavan
				   glm::rotate(glm::mat4(1.0f), 3*PI/2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			       glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(useTextureLocation, 0);
		glUniform1i(codColLocation, 0);
		glUniform1f(materialSpecularStrengthLoc, 1.0f);
		glUniform1f(materialShininessLoc, 128.0f);

		// Activare VAO
		glBindVertexArray(VaoIdObj2);
		// Dezactivam citirea culorii din VBO (care oricum nu exista in OBJ)
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 1.0f, 1.0f, 0.9f, 0.4f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj2);

		// Reactivare atributul 1 pentru restul obiectelor din scena
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	glDisable(GL_BLEND);


	// DESENARE MODEL OBJ 5 (Vaza)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (nrVerticesObj5 > 0) {
		// Matricea de modelare
		myMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 55.0f)) * //fix pe masa
			glm::rotate(glm::mat4(1.0f),  PI / 2, glm::vec3(1.0f, 0.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(6.5f, 6.5f, 6.5f));
		glUniformMatrix4fv(myMatrixLocation, 1, GL_FALSE, glm::value_ptr(myMatrix));

		// Setari shader
		glUniform1i(useTextureLocation, 0);
		glUniform1i(codColLocation, 0);
		glUniform1f(materialSpecularStrengthLoc, 1.0f);
		glUniform1f(materialShininessLoc, 128.0f);

		// Activare VAO
		glBindVertexArray(VaoIdObj5);
		// Dezactivam citirea culorii din VBO (care oricum nu exista in OBJ)
		glDisableVertexAttribArray(1);
		glVertexAttrib4f(1, 1.0f, 0.45f, 0.55f, 0.75f);

		// Desenare
		glDrawArrays(GL_TRIANGLES, 0, nrVerticesObj5);

		// Reactivare atributul 1 pentru restul obiectelor din scena
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	}

	glDisable(GL_BLEND);

	glutSwapBuffers();
	glFlush();
}
void Cleanup(void)
{
	DestroyShaders();
	DestroyVBO();
}
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1200, 900);
	glutCreateWindow("Amestecare in context 3D");
	glewInit();
	Initialize();
	glutIdleFunc(RenderFunction);
	glutDisplayFunc(RenderFunction);
	glutKeyboardFunc(processNormalKeys);
	glutSpecialFunc(processSpecialKeys);
	glutCloseFunc(Cleanup);
	glutMainLoop();
}