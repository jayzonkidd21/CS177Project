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

const GLuint COLOR_BROWN = 0x003366, COLOR_GREEN = 0x33FF00, COLOR_RED = 0x000099, COLOR_BLUE = 0xCC0000, COLOR_YELLOW = 0x33FFFF, COLOR_ORANGE =0x0033FF, COLOR_VIOLET = 0x660066, COLOR_GREY = 0x666666;
//BROWN, RED, BLUE, ORANGE, GREY, VIOLET, ORANGE, YELLOW, GREEN 
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
	
	//RectangleNode houseBody( 0.4, 0.4, 0.0, 0.0, COLOR_YELLOW );
	//TriangleNode roof( 0.8, 0.35, 0.0, 0.375, COLOR_BROWN );

	RectangleNode houseBody( 200, 200, 0.0, 0.0 - 220, COLOR_RED );
	TriangleNode roof( 200, 100, 0, 150 - 220, COLOR_BROWN );

	CircleNode sun( 50, 240, 240, COLOR_YELLOW );

	CircleNode guyHead( 20, 150, -240, COLOR_YELLOW );
	RectangleNode guyBody( 80, 20, 150, -280, COLOR_BLUE );

	RectangleNode xmasTreeBody( 60, 40, -240, -290, COLOR_BROWN );
	TriangleNode xmasTreeLeaf( 100, 80, -240, -220, COLOR_GREEN );

	guy.children.push_back( &guyBody );
	guy.children.push_back( &guyHead );

	house.children.push_back( &roof );
	house.children.push_back( &houseBody );

	xmasTree.children.push_back( &xmasTreeBody );
	xmasTree.children.push_back( &xmasTreeLeaf );

	scene.children.push_back( &house );
	scene.children.push_back( &xmasTree );

	root.children.push_back( &scene );
	root.children.push_back( &sun );
	root.children.push_back( &guy );

	mvpMatrixID = glGetUniformLocation( mainProgram, "mvpMatrix" );
	double t = 0;

	GLfloat camX = 150, camY = 0, camS = 1;

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
				camS += 0.02;
			else
				camY += 5;
		}
		else if( glfwGetKey( GLFW_KEY_DOWN ) == GLFW_PRESS )
		{
			if( lShiftPressed )
				camS -= 0.02;
			else
				camY -= 5;
		}
		else if( glfwGetKey( GLFW_KEY_LEFT ) == GLFW_PRESS )
			camX -= 5;
		else if( glfwGetKey( GLFW_KEY_RIGHT ) == GLFW_PRESS )
			camX += 5;
			
		if( camS < 0 )
			camS = 0;

		if( camY < 0 )
			camY = 0;
			
        GLMatrix3 modelMatrix, transMatrix, tempMatrix;

		modelMatrix.setTranslation( -camX, -camY );
		scene.transform = modelMatrix;

		//root.transform = modelMatrix;
		
		tempMatrix.setClipMatrix( -( width / 2), width / 2, height/2, -( height / 2 ) ); 
		modelMatrix = tempMatrix;
		root.draw( modelMatrix );
        
		t += 0.02;
		//VERY IMPORTANT: displays the buffer to the screen
		glfwSwapBuffers();
	} while ( glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS &&
			glfwGetWindowParam(GLFW_OPENED) );

	glDeleteProgram(mainProgram);
	glfwTerminate();
	return 0;
}
