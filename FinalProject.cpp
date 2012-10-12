#include <GL/glew.h>
#include <GL/glfw.h>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <vector>

using namespace std;

static const double MY_PI = 3.14159265358979323846264338327;

enum { ATTRIB_POS, ATTRIB_COLOR };

const GLuint COLOR_BROWN = 0x003366, COLOR_GREEN = 0x33FF00, COLOR_RED = 0x000099, COLOR_BLUE = 0xCC0000, COLOR_YELLOW = 0x33FFFF, COLOR_ORANGE =0x0033FF, COLOR_VIOLET = 0x660066, COLOR_GREY = 0x666666, COLOR_WHITE = 0xFFFFFF, COLOR_BLACK = 0x000000, COLOR_LCYAN= 0xE0FFFF;
//BROWN, RED, BLUE, ORANGE, GREY, VIOLET, ORANGE, YELLOW, GREEN, BLACK, WHITE 
GLuint mvpMatrixID;

struct Vtx
{
	GLfloat x, y;
	GLuint color;
};

/********************
 *
 * 3x3 OpenGL Matrix class
 *
 ********************/
struct GLMatrix3 {
	GLfloat mat[9];
	
	void setIdentity() {
		mat[0] = 1, mat[3] = 0, mat[6] = 0;
		mat[1] = 0, mat[4] = 1, mat[7] = 0;
		mat[2] = 0, mat[5] = 0, mat[8] = 1;
	}
	
	void setRotation(GLfloat x, GLfloat y, GLfloat theta) {
		const GLfloat c = cos(theta), s = sin(theta);
		mat[0] = c, mat[3] = -s, mat[6] = -c * x + s * y + x;
		mat[1] = s, mat[4] = c,  mat[7] = -s * x - c * y + y;
		mat[2] = 0, mat[5] = 0,  mat[8] = 1;
	}
	
	void setTranslation(GLfloat x, GLfloat y) {
		mat[0] = 1, mat[3] = 0, mat[6] = x;
		mat[1] = 0, mat[4] = 1, mat[7] = y;
		mat[2] = 0, mat[5] = 0, mat[8] = 1;
	}

	void translate(GLfloat x, GLfloat y) {
		mat[6] += x;
		mat[7] += y;
	}
	
	void scale(GLfloat sx, GLfloat sy) {
		mat[0] *= sx;
		mat[3] *= sx;
		mat[6] *= sx;
		
		mat[1] *= sy;
		mat[4] *= sy;
		mat[7] *= sy;
	}
	
	void transpose() {
		swap(mat[3],mat[1]);
		swap(mat[6],mat[2]);
		swap(mat[7],mat[5]);
	}

	void setClipMatrix( GLfloat lBound, GLfloat rBound, GLfloat topBound, GLfloat botBound )
	{
		setIdentity();
		mat[0] = 2 / ( rBound - lBound );
		mat[4] = 2 / ( topBound - botBound );
	}
	
	GLMatrix3& operator=(const GLMatrix3 &rhs) {
		memcpy(mat, rhs.mat, sizeof(mat));
		return *this;
	}
	
	GLMatrix3 operator*(const GLMatrix3 &rhs) const {
		GLMatrix3 ret;
		for ( int i = 0; i < 9; ++i ) {
			const int a = i % 3, b = (i / 3) * 3;
			ret.mat[i] = mat[a]*rhs.mat[b] + mat[a+3]*rhs.mat[b+1] + mat[a+6]*rhs.mat[b+2];
		}
		return ret;
	}
	
	GLMatrix3& operator*=(const GLMatrix3 &rhs) {
		GLMatrix3 tmp;
		for ( int i = 0; i < 9; ++i ) {
			const int a = i % 3, b = (i / 3) * 3;
			tmp.mat[i] = mat[a]*rhs.mat[b] + mat[a+3]*rhs.mat[b+1] + mat[a+6]*rhs.mat[b+2];
		}
		memcpy(mat, tmp.mat, sizeof(mat));
		return *this;
	}
};

/********************
 *
 * Scene Node class used to implement a transformation hierarchy.
 *
 ********************/
class SceneNode {
public:
	GLMatrix3 transform;
	vector<SceneNode*> children;
	SceneNode() {
		transform.setIdentity();
	}
	virtual void draw(const GLMatrix3 &parentTransform) {
		drawChildren(parentTransform * transform);
	}
	
	virtual void update(double t) {
		for ( size_t i = 0; i < children.size(); ++i )
			children[i]->update(t);
	}
	
	void drawChildren(const GLMatrix3 &t) {
		for ( size_t i = 0; i < children.size(); ++i )
			children[i]->draw(t);
	}
	
	virtual ~SceneNode() {
	}
};

class RectangleNode : public SceneNode
{
	Vtx vertices[6];
    public:
	RectangleNode( GLfloat length, GLfloat width, GLfloat centerX, GLfloat centerY, GLuint cColor )
	{
		vertices[0].x = centerX - width/2;
		vertices[0].y = centerY + length/2;
		vertices[1].x = centerX - width/2;
		vertices[1].y = centerY - length/2;
		vertices[2].x = centerX + width/2;
		vertices[2].y = centerY - length/2;
		vertices[3].x = centerX + width/2;
		vertices[3].y = centerY - length/2;
		vertices[4].x = centerX + width/2;
		vertices[4].y = centerY + length/2;
		vertices[5].x = centerX - width/2;
		vertices[5].y = centerY + length/2;

		for( int i = 0; i < 6; i++ )
		{
			vertices[i].color = cColor;
		}
	}

	virtual void draw(const GLMatrix3 &parentTransform) {
		const GLMatrix3 &t = parentTransform * transform;
		glVertexAttribPointer(ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), &vertices[0].x);
		glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), &vertices[0].color);
		glUniformMatrix3fv(mvpMatrixID, 1, false, t.mat);
		glDrawArrays(GL_TRIANGLES, 0, sizeof( vertices ) / sizeof( Vtx ) );
		
		drawChildren(t);
	}
};

class CircleNode : public SceneNode
{
      Vtx vertices[360];
      public:
      CircleNode( GLfloat radius, GLfloat centerX, GLfloat centerY, GLuint cColor )
      {
			for( int i = 0; i < 360; i++ )
			{
				float angleInRadians = i * MY_PI / 180;
				vertices[i].x = centerX + radius * cos( angleInRadians );	
				vertices[i].y = centerY + radius * sin( angleInRadians );
				vertices[i].color = cColor;
			}
	  }
	  
	  virtual void draw(const GLMatrix3 &parentTransform) {
		const GLMatrix3 &t = parentTransform * transform;
		glVertexAttribPointer(ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), &vertices[0].x);
		glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), &vertices[0].color);
		glUniformMatrix3fv(mvpMatrixID, 1, false, t.mat);
		glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof( vertices ) / sizeof( Vtx ) );
		
		drawChildren(t);
	}
};

class TriangleNode : public SceneNode
{
	Vtx vertices[3];
	public:
	TriangleNode( GLfloat base, GLfloat height, GLfloat centerX, GLfloat centerY, GLuint cColor )
	{
		vertices[0].x = centerX;
		vertices[0].y = centerY + height/2;
		vertices[1].x = centerX - base/2;
		vertices[1].y = centerY - height/2;
		vertices[2].x = centerX + base/2;
		vertices[2].y = centerY - height/2;

		for( int i = 0; i < 3; i++ )
		{
			vertices[i].color = cColor;
		}
	}

	virtual void draw(const GLMatrix3 &parentTransform) {
		const GLMatrix3 &t = parentTransform * transform;
		glVertexAttribPointer(ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), &vertices[0].x);
		glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), &vertices[0].color);
		glUniformMatrix3fv(mvpMatrixID, 1, false, t.mat);
		glDrawArrays(GL_TRIANGLES, 0, sizeof( vertices ) / sizeof( Vtx ) );
		
		drawChildren(t);
	}
};

class HardRectNode : public SceneNode
{
	Vtx vertices[6];
	public:
	HardRectNode( GLfloat x1, GLfloat y1,GLfloat x2, GLfloat y2,GLfloat x3, GLfloat y3,GLfloat x4, GLfloat y4, GLfloat centerX, GLfloat centerY, GLuint cColor )
	{
		vertices[0].x = x1;
		vertices[0].y = y1;
		vertices[1].x = x2;
		vertices[1].y = y2;
		vertices[2].x = x3;
		vertices[2].y = y3;

		vertices[3].x = x3;
		vertices[3].y = y3;
		vertices[4].x = x4;
		vertices[4].y = y4;
		vertices[5].x = x1;
		vertices[5].y = y1;

		for( int i = 0; i < 6; i++ )
		{
			vertices[i].color = cColor;
		}
	}

	virtual void draw(const GLMatrix3 &parentTransform) {
		const GLMatrix3 &t = parentTransform * transform;
		glVertexAttribPointer(ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), &vertices[0].x);
		glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vtx), &vertices[0].color);
		glUniformMatrix3fv(mvpMatrixID, 1, false, t.mat);
		glDrawArrays(GL_TRIANGLES, 0, sizeof( vertices ) / sizeof( Vtx ) );
		
		drawChildren(t);
	}
};

GLuint mainProgram = 0;

bool loadShaderSource(GLuint shader, const char *path) {
	FILE *f = fopen(path, "r");
	if ( !f ) {
		std::cerr << "ERROR: shader source not found: " << path << '\n';
		return false;
	}
	fseek(f, 0, SEEK_END);
	vector<char> sauce(ftell(f) + 1);
	fseek(f, 0, SEEK_SET);
	fread(&sauce[0], 1, sauce.size(), f);
	fclose(f);
	const GLchar *ptr = &sauce[0];
	glShaderSource(shader, 1, &ptr, 0);
	if ( glGetError() != GL_NO_ERROR ) {
		std::cerr << "ERROR: Unable to load shader\n";
		return false;
	}
	return true;
}

void checkShaderStatus(GLuint shader) {
	GLint logLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0) {
		GLchar *log = (GLchar *)malloc(logLength);
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		std::cout << "Shader compile log:\n" << log << endl;
		free(log);
	}
}

void initShader()
{
	GLuint fShader = glCreateShader( GL_FRAGMENT_SHADER );
	GLuint vShader = glCreateShader( GL_VERTEX_SHADER );

	loadShaderSource( fShader, "project.fsh" );
	loadShaderSource( vShader, "project.vsh" );

	glCompileShader( fShader );
	checkShaderStatus( fShader );

	glCompileShader( vShader );
	checkShaderStatus( vShader );

	mainProgram = glCreateProgram();

	glAttachShader( mainProgram, vShader );
	glAttachShader( mainProgram, fShader );

	glBindAttribLocation( mainProgram, ATTRIB_POS, "position" );
	glBindAttribLocation( mainProgram, ATTRIB_COLOR, "color" );

	glLinkProgram( mainProgram );

	glDeleteShader( fShader );
	glDeleteShader( vShader );
}

int main()
{
	if ( !glfwInit() ) {
		std::cerr << "Unable to initialize OpenGL!\n";
		return -1;
	}
	
	if ( !glfwOpenWindow(640,640, //width and height of the screen
				8,8,8,8, //Red, Green, Blue and Alpha bits
				0,0, //Depth and Stencil bits
				GLFW_WINDOW)) {
		std::cerr << "Unable to create OpenGL window.\n";
		glfwTerminate();
		return -1;
	}

	if ( glewInit() != GLEW_OK ) {
		std::cerr << "Unable to hook OpenGL extensions!\n";
		return -1;
	}

	glfwSetWindowTitle("GLFW Simple Example");

	// Ensure we can capture the escape key being pressed below
	glfwEnable( GLFW_STICKY_KEYS );

	// Enable vertical sync (on cards that support it)
	glfwSwapInterval( 1 );

	glClearColor(0,0,0,0);

	initShader();

	glEnableVertexAttribArray( ATTRIB_POS );
	glEnableVertexAttribArray( ATTRIB_COLOR );

	/*glVertexAttribPointer( ATTRIB_POS, 2, GL_FLOAT, GL_FALSE, sizeof( Vtx ), &triangle[0].x );
	glVertexAttribPointer( ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Vtx ), &triangle[0].color );*/

	glUseProgram( mainProgram );

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	SceneNode root;
	SceneNode scene;
	SceneNode house;
	SceneNode guy;
	SceneNode xmasTree;
	SceneNode cloud;
	SceneNode airplane;
	
	
	//RectangleNode houseBody( 0.4, 0.4, 0.0, 0.0, COLOR_YELLOW );
	//TriangleNode roof( 0.8, 0.35, 0.0, 0.375, COLOR_BROWN );
	
	RectangleNode cloudBulk(60, 200, -125, 215, COLOR_LCYAN );
	CircleNode cloudC1( 20, -235, 200, COLOR_LCYAN);
	CircleNode cloudC2( 25, -215, 230, COLOR_LCYAN);
	CircleNode cloudC3( 20, -185, 250, COLOR_LCYAN);
	CircleNode cloudC4( 30, -145, 240, COLOR_LCYAN);
	CircleNode cloudC5( 30, -105, 270, COLOR_LCYAN);
	CircleNode cloudC6( 25, -95, 260, COLOR_LCYAN);
	CircleNode cloudC7( 35, -45, 225, COLOR_LCYAN);
	CircleNode cloudC8( 25, -15, 215, COLOR_LCYAN);
	CircleNode cloudC9( 30, -35, 200, COLOR_LCYAN);
	CircleNode cloudC10( 40, -75, 220, COLOR_LCYAN);
	CircleNode cloudC11( 30, -115, 205, COLOR_LCYAN);
	CircleNode cloudC12( 25, -165, 200, COLOR_LCYAN);
	CircleNode cloudC13( 25, -205, 200, COLOR_LCYAN);
	cloud.children.push_back( &cloudBulk );
	cloud.children.push_back( &cloudC1 );
	cloud.children.push_back( &cloudC2 );
	cloud.children.push_back( &cloudC3 );
	cloud.children.push_back( &cloudC4 );
	cloud.children.push_back( &cloudC5 );
	cloud.children.push_back( &cloudC6 );
	cloud.children.push_back( &cloudC7 );
	cloud.children.push_back( &cloudC8 );
	cloud.children.push_back( &cloudC9 );
	cloud.children.push_back( &cloudC10 );
	cloud.children.push_back( &cloudC11 );
	cloud.children.push_back( &cloudC12 );
	cloud.children.push_back( &cloudC13 );

	RectangleNode cloudBulk2(60, 200, -125 + 290, 215 -115, COLOR_LCYAN );
	CircleNode cloudC1x( 20, -235 + 290, 200 -115, COLOR_LCYAN);
	CircleNode cloudC2x( 25, -215 + 290, 230 -115, COLOR_LCYAN);
	CircleNode cloudC3x( 20, -185 + 290, 250 -115, COLOR_LCYAN);
	CircleNode cloudC4x( 30, -145 + 290, 240 -115, COLOR_LCYAN);
	CircleNode cloudC5x( 30, -105 + 290, 270 -115, COLOR_LCYAN);
	CircleNode cloudC6x( 25, -95 + 290, 260 -115, COLOR_LCYAN);
	CircleNode cloudC7x( 35, -45 + 290, 225 -115, COLOR_LCYAN);
	CircleNode cloudC8x( 25, -15 + 290, 215 -115, COLOR_LCYAN);
	CircleNode cloudC9x( 30, -35 + 290, 200 -115, COLOR_LCYAN);
	CircleNode cloudC10x( 40, -75 + 290, 220 -115, COLOR_LCYAN);
	CircleNode cloudC11x( 30, -115 + 290, 205 -115, COLOR_LCYAN);
	CircleNode cloudC12x( 25, -165 + 290, 200 -115, COLOR_LCYAN);
	CircleNode cloudC13x( 25, -205 + 290, 200 -115, COLOR_LCYAN);
	cloud.children.push_back( &cloudBulk2 );
	cloud.children.push_back( &cloudC1x );
	cloud.children.push_back( &cloudC2x );
	cloud.children.push_back( &cloudC3x );
	cloud.children.push_back( &cloudC4x );
	cloud.children.push_back( &cloudC5x );
	cloud.children.push_back( &cloudC6x );
	cloud.children.push_back( &cloudC7x );
	cloud.children.push_back( &cloudC8x );
	cloud.children.push_back( &cloudC9x );
	cloud.children.push_back( &cloudC10x );
	cloud.children.push_back( &cloudC11x );
	cloud.children.push_back( &cloudC12x );
	cloud.children.push_back( &cloudC13x );

	RectangleNode houseBody( 195, 200, 0.0, 0.0 - 220, COLOR_RED );
	RectangleNode houseBodyBorder( 200, 200, 0.0, 0.0 - 220, COLOR_BLACK);
	//RectangleNode doorBorder( 150, 80, 0.0, 0.0 - 275, COLOR_BLACK );
	//RectangleNode door( 145, 75, 0.0, 0.0 - 275, COLOR_YELLOW);
	RectangleNode doorBorder( 120, 80, 0.0, 0.0 - 255, COLOR_BLACK );
	RectangleNode door( 125, 75, 0.0, 0.0 - 255, COLOR_YELLOW);
	CircleNode doorKnob( 7, 0.0 + 20, 0.0 -275, COLOR_BLACK );
	TriangleNode roof( 250, 100, 0.0, 150 - 220, COLOR_BROWN );
	TriangleNode roofCover( 125, 40, 0.0, 150 - 190, COLOR_BLACK );

	CircleNode sun( 50, 240, 240, COLOR_YELLOW );
	HardRectNode sunLight1(80 , 240, 110, 233, 180, 240, 110, 247, 140, 260, COLOR_YELLOW );
	//HardRectNode sunLight5(240 - (120/sqrt(2.0)) , 240 + (20/sqrt(2.0)), 240 - (150/sqrt(2.0)), 240 +(27/sqrt(2.0)), 240 - (80/sqrt(2.0)), 240 +(20/sqrt(2.0)) , 240+(150/sqrt(2.0)), 240 +(13/sqrt(2.0)), 140, 260, COLOR_YELLOW );
	//HardRectNode sunLight6(240 - (120/sqrt(2.0)) +20, 240 + (20/sqrt(2.0))+25, 240 - (150/sqrt(2.0))+20, 240 +(27/sqrt(2.0))+25, 240 - (80/sqrt(2.0))+20, 240 +(20/sqrt(2.0)) +25, 240+(150/sqrt(2.0))+20, 240 +(13/sqrt(2.0))+25, 140, 260, COLOR_YELLOW );
	//HardRectNode sunLight7(240 - (120/sqrt(2.0)) +20 , 240 + (20/sqrt(2.0))-60, 240 - (150/sqrt(2.0)) +20, 240 +(27/sqrt(2.0))-60, 240 - (80/sqrt(2.0)) +20, 240 +(20/sqrt(2.0))-60 , 240+(150/sqrt(2.0)) +20, 240 +(13/sqrt(2.0))-60, 140, 260, COLOR_YELLOW );
	//HardRectNode sunLight8(240 - (120/sqrt(2.0)), 240 + (20/sqrt(2.0))-35, 240 - (150/sqrt(2.0)), 240 +(27/sqrt(2.0))-35, 240 - (80/sqrt(2.0)), 240 +(20/sqrt(2.0)) -35, 240+(150/sqrt(2.0)), 240 +(13/sqrt(2.0))-35, 140, 260, COLOR_YELLOW );
	HardRectNode sunLight4(240 , 80, 233, 110, 240, 180, 247, 110, 260, 140, COLOR_YELLOW );
	HardRectNode sunLight2(400 , 240, 370, 233, 300, 240, 370, 247, 140, 260, COLOR_YELLOW );
	HardRectNode sunLight3(240 , 400, 233, 370, 240, 300, 247, 370, 140, 260, COLOR_YELLOW );

	TriangleNode airp( 60, 60, -320, 240, COLOR_BLUE );
	TriangleNode airWingLeft( 20, 45, -320, 250, COLOR_BLACK );
	TriangleNode airWingLeft2( 17, 42, -320, 250, 0x9D9D0C ); // 
	RectangleNode airpHollow( 20, 20, -320, 220, COLOR_BLACK );
	
	CircleNode guyHead( 20, 150, -240, COLOR_YELLOW );
	RectangleNode guyBody( 80, 20, 150, -280, COLOR_BLUE );

	RectangleNode xmasTreeBody( 60, 40, -240, -290, COLOR_BROWN );
	TriangleNode xmasTreeLeaf( 100, 80, -240, -220, COLOR_GREEN );
	TriangleNode xmasTreeLeaf2( 90, 70, -240, -210, COLOR_GREEN );
	TriangleNode xmasTreeLeaf2S( 90, 70, -240, -212, COLOR_BLACK );
	TriangleNode xmasTreeLeaf3( 80, 60, -240, -200, COLOR_GREEN );
	TriangleNode xmasTreeLeaf3S(80, 60, -240, -202, COLOR_BLACK );
	TriangleNode xmasTreeLeaf4( 70, 50, -240, -190, COLOR_GREEN );
	TriangleNode xmasTreeLeaf4S( 70, 50, -240, -192, COLOR_BLACK );
	TriangleNode xmasTreeLeaf5( 60, 40, -240, -180, COLOR_GREEN );
	TriangleNode xmasTreeLeaf5S( 60, 40, -240, -182, COLOR_BLACK );
	TriangleNode xmasTreeLeaf6( 50, 30, -240, -170, COLOR_GREEN );
	TriangleNode xmasTreeLeaf6S( 50, 30, -240, -172, COLOR_BLACK );

	RectangleNode xmasTreeBodyA( 60, 40, 170, -290, COLOR_BROWN );
	TriangleNode xmasTreeLeafA( 100, 80, 170, -220, COLOR_GREEN );
	TriangleNode xmasTreeLeaf2A( 90, 70, 170, -210, COLOR_GREEN );
	TriangleNode xmasTreeLeaf2SA( 90, 70, 170, -212, COLOR_BLACK );
	TriangleNode xmasTreeLeaf3A( 80, 60, 170, -200, COLOR_GREEN );
	TriangleNode xmasTreeLeaf3SA(80, 60, 170, -202, COLOR_BLACK );
	TriangleNode xmasTreeLeaf4A( 70, 50, 170, -190, COLOR_GREEN );
	TriangleNode xmasTreeLeaf4SA( 70, 50, 170, -192, COLOR_BLACK );
	TriangleNode xmasTreeLeaf5A( 60, 40, 170, -180, COLOR_GREEN );
	TriangleNode xmasTreeLeaf5SA( 60, 40, 170, -182, COLOR_BLACK );
	TriangleNode xmasTreeLeaf6A( 50, 30, 170, -170, COLOR_GREEN );
	TriangleNode xmasTreeLeaf6SA( 50, 30, 170, -172, COLOR_BLACK );

	RectangleNode slenderBody( 100, 30, -400, 50, 0xFFFFFFFF );

	guy.children.push_back( &guyBody );
	guy.children.push_back( &guyHead );

	house.children.push_back( &roof );
	house.children.push_back( &roofCover );
	house.children.push_back( &houseBodyBorder );
	house.children.push_back( &houseBody );
	house.children.push_back( &doorBorder );
	house.children.push_back( &door );
	house.children.push_back( &doorKnob );

	xmasTree.children.push_back( &xmasTreeBody );
	xmasTree.children.push_back( &xmasTreeLeaf );
	xmasTree.children.push_back( &xmasTreeLeaf2S );
	xmasTree.children.push_back( &xmasTreeLeaf2 );	
	xmasTree.children.push_back( &xmasTreeLeaf3S );
	xmasTree.children.push_back( &xmasTreeLeaf3 );	
	xmasTree.children.push_back( &xmasTreeLeaf4S );
	xmasTree.children.push_back( &xmasTreeLeaf4 );	
	xmasTree.children.push_back( &xmasTreeLeaf5S );
	xmasTree.children.push_back( &xmasTreeLeaf5 );	
	xmasTree.children.push_back( &xmasTreeLeaf6S );
	xmasTree.children.push_back( &xmasTreeLeaf6 );

	xmasTree.children.push_back( &xmasTreeBodyA );
	xmasTree.children.push_back( &xmasTreeLeafA );
	xmasTree.children.push_back( &xmasTreeLeaf2SA );
	xmasTree.children.push_back( &xmasTreeLeaf2A );	
	xmasTree.children.push_back( &xmasTreeLeaf3SA );
	xmasTree.children.push_back( &xmasTreeLeaf3A );	
	xmasTree.children.push_back( &xmasTreeLeaf4SA );
	xmasTree.children.push_back( &xmasTreeLeaf4A );	
	xmasTree.children.push_back( &xmasTreeLeaf5SA );
	xmasTree.children.push_back( &xmasTreeLeaf5A );	
	xmasTree.children.push_back( &xmasTreeLeaf6SA );
	xmasTree.children.push_back( &xmasTreeLeaf6A );

	airplane.children.push_back( &airp );
	airplane.children.push_back( &airWingLeft );
	airplane.children.push_back( &airWingLeft2 );
	airplane.children.push_back( &airpHollow );
	
	scene.children.push_back( &airplane );
	scene.children.push_back( &house );
	scene.children.push_back( &xmasTree );
	scene.children.push_back( &cloud );
	
	sun.children.push_back( &sunLight1 );
	sun.children.push_back( &sunLight2 );
	sun.children.push_back( &sunLight3 );
	sun.children.push_back( &sunLight4 );

	root.children.push_back( &sun );
	root.children.push_back( &scene );
	root.children.push_back( &guy );
	
	mvpMatrixID = glGetUniformLocation( mainProgram, "mvpMatrix" );
	GLuint timeId = glGetUniformLocation( mainProgram, "t" );
	double t = 0;
	double time = 1;

	GLfloat camX = 150, camY = 0, camS = 320, camR = 0;

	do {
		int width, height;
		// Get window size (may be different than the requested size)
		//we do this every frame to accommodate window resizing.
		glfwGetWindowSize( &width, &height );
		glViewport( 0, 0, width, height );

		glClear(GL_COLOR_BUFFER_BIT);
		
		bool lShiftPressed = glfwGetKey( GLFW_KEY_LSHIFT ) == GLFW_PRESS;

		if( glfwGetKey( GLFW_KEY_UP ) == GLFW_PRESS )
		{
			if( lShiftPressed )
				camS += 5;
			else
				camY += 5;
		}
		else if( glfwGetKey( GLFW_KEY_DOWN ) == GLFW_PRESS )
		{
			if( lShiftPressed )
				camS -= 5;
			else
				camY -= 5;
		}
		else if( glfwGetKey( GLFW_KEY_LEFT ) == GLFW_PRESS )
		{
			if( lShiftPressed )
				camR += 0.05;
			else
				camX -= 5;
		}
		else if( glfwGetKey( GLFW_KEY_RIGHT ) == GLFW_PRESS )
		{
			if( lShiftPressed )
				camR -= 0.05;
			else
				camX += 5;
		}
			
		if( camS < 0 )
			camS = 0;

		if( camY < 0 )
			camY = 0;

		if( camY > 50 )
			camY = 50;

		if( camX < -420 )
			camX = -420;
		else if( camX > 240 )
			camX = 240;
			
        GLMatrix3 modelMatrix, transMatrix, tempMatrix;

		transMatrix.setRotation( -320 + 40 * sin(t), 240, t );
		airplane.transform = transMatrix;
		//airplane.transform = transMatrix * airplane.transform;

		transMatrix.setRotation( 0, 0, t );
		sun.transform = transMatrix;

		modelMatrix.setTranslation( -camX, -camY );
		scene.transform = modelMatrix;

		//root.transform = modelMatrix;
		
		tempMatrix.setClipMatrix( -320, 320,320, -320 ); 
		modelMatrix = tempMatrix;
		tempMatrix.scale( camS, camS );
		modelMatrix = tempMatrix * modelMatrix;
		tempMatrix.setIdentity();
		tempMatrix.setRotation( 0, 0, -camR );
		modelMatrix *= tempMatrix;
		root.draw( modelMatrix );
        
		time += 0.02;
		glUniform1f(timeId, time);

		t += 0.02;
		//VERY IMPORTANT: displays the buffer to the screen
		glfwSwapBuffers();
	} while ( glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS &&
			glfwGetWindowParam(GLFW_OPENED) );

	glDeleteProgram(mainProgram);
	glfwTerminate();
	return 0;
}
