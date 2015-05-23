#include "glew\glew.h"
#include "glfw\glfw3.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtc\type_ptr.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// This is your reference to your shader program.
// This will be assigned with glCreateProgram().
// This program will run on your GPU.
GLuint program;

// These are your references to your actual compiled shaders
GLuint vertex_shader;
GLuint fragment_shader;

// This is a reference to your uniform MVP matrix in your vertex shader
GLuint uniMVP;

// A reference to the data we will use for vertex locations.
GLuint positionBuffer;
GLuint positionBufferTexture;

// A reference to the data that tells us what order to use the vertex points in.
// This is useful, since it allows us to re-use vertex data when it is duplicated. 
// Many things use such triangle index lists, to the point where there are some special
// API features relating to them which will be seen in a future example.
GLuint triangleBuffer;
GLuint triangleBufferTexture;

// Number of triangles we find in the model file.
int numTriangles;

// These are 4x4 transformation matrices, which you will locally modify before passing into the vertex shader via uniMVP
glm::mat4 trans;
glm::mat4 proj;
glm::mat4 view;
glm::mat4 MVP;


// This runs once a frame, before renderScene
void update()
{
    static float orbit = 0;
    orbit += 0.00025f;
    const float radius = 30.5f;
    view = glm::lookAt( glm::vec3( sin( orbit )*radius, 0.0f, -cos( orbit )*radius ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );

	// This takes the value of our transformation, view, and projection matrices and multiplies them together to create the MVP matrix.
	// Then it sets our uniform MVP matrix within our shader to this value.
	// Parameters are: Location within the shader, size (in case we're passing in multiple matrices via a single pointer), whether or not to transpose the matrix, and a pointer 
	// to the matrix value we're passing in.
	MVP = proj * view * trans;
	glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(MVP));
}

// This function runs every frame
void renderScene()
{
	// Clear the color buffer and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Clear the screen to a neutral gray
	glClearColor(0.5, 0.5, 0.5, 1.0);

	// Tell OpenGL to use the shader program you've created.
    glUseProgram( program );



    // Bind our texture to this slot. In this demo, it does not actually need to be set every frame, since we have no other textures in our rendering. But if we
    // rendered a different texture from the same slot, we would need to rebind the texture in this slot. Same as with setting the shader program to use.
    // This is an important detail since a lot of switching out shaders and bound resources such as textures can have a high overhead cost. 
    // Note that we need to bind the texture with the correct texture type (GL_TEXTURE_BUFFER in this case).
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, positionBufferTexture );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, triangleBufferTexture );

	// Draw the triangles.
    glDrawArrays( GL_TRIANGLES, 0, 3 * numTriangles );
}

// This method reads the text from a file.
// Realistically, we wouldn't want plain text shaders hardcoded in, we'd rather read them in from a separate file so that the shader code is separated.
std::string readShader(std::string fileName)
{
	std::string shaderCode;
	std::string line;

	// We choose ifstream and std::ios::in because we are opening the file for input into our program.
	// If we were writing to the file, we would use ofstream and std::ios::out.
	std::ifstream file(fileName, std::ios::in);

	// This checks to make sure that we didn't encounter any errors when getting the file.
	if (!file.good())
	{
		std::cout << "Can't read file: " << fileName.data() << std::endl;

		// Return so we don't error out.
		return "";
	}

	// ifstream keeps an internal "get" position determining the location of the element to be read next
	// seekg allows you to modify this location, and tellg allows you to get this location
	// This location is stored as a streampos member type, and the parameters passed in must be of this type as well
	// seekg parameters are (offset, direction) or you can just use an absolute (position).
	// The offset parameter is of the type streamoff, and the direction is of the type seekdir (an enum which can be ios::beg, ios::cur, or ios::end referring to the beginning, 
	// current position, or end of the stream).
	file.seekg(0, std::ios::end);					// Moves the "get" position to the end of the file.
	shaderCode.resize((unsigned int)file.tellg());	// Resizes the shaderCode string to the size of the file being read, given that tellg will give the current "get" which is at the end of the file.
	file.seekg(0, std::ios::beg);					// Moves the "get" position to the start of the file.

	// File streams contain two member functions for reading and writing binary data (read, write). The read function belongs to ifstream, and the write function belongs to ofstream.
	// The parameters are (memoryBlock, size) where memoryBlock is of type char* and represents the address of an array of bytes are to be read from/written to.
	// The size parameter is an integer that determines the number of characters to be read/written from/to the memory block.
	file.read(&shaderCode[0], shaderCode.size());	// Reads from the file (starting at the "get" position which is currently at the start of the file) and writes that data to the beginning
	// of the shaderCode variable, up until the full size of shaderCode. This is done with binary data, which is why we must ensure that the sizes are all correct.

	file.close(); // Now that we're done, close the file and return the shaderCode.

	return shaderCode;
}

// This method will consolidate some of the shader code we've written to return a GLuint to the compiled shader.
// It only requires the shader source code and the shader type.
GLuint createShader(std::string sourceCode, GLenum shaderType)
{
	// glCreateShader, creates a shader given a type (such as GL_VERTEX_SHADER) and returns a GLuint reference to that shader.
	GLuint shader = glCreateShader(shaderType);
	const char *shader_code_ptr = sourceCode.c_str(); // We establish a pointer to our shader code string
	const int shader_code_size = sourceCode.size();   // And we get the size of that string.

	// glShaderSource replaces the source code in a shader object
	// It takes the reference to the shader (a GLuint), a count of the number of elements in the string array (in case you're passing in multiple strings), a pointer to the string array 
	// that contains your source code, and a size variable determining the length of the array.
	glShaderSource(shader, 1, &shader_code_ptr, &shader_code_size);
	glCompileShader(shader); // This just compiles the shader, given the source code.

	GLint isCompiled = 0;

	// Check the compile status to see if the shader compiled correctly.
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		char infolog[1024];
		glGetShaderInfoLog(shader, 1024, NULL, infolog);

		// Print the compile error.
		std::cout << "The shader failed to compile with the error:" << std::endl << infolog << std::endl;

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(shader); // Don't leak the shader.

		// NOTE: I almost always put a break point here, so that instead of the program continuing with a deleted/failed shader, it stops and gives me a chance to look at what may 
		// have gone wrong. You can check the console output to see what the error was, and usually that will point you in the right direction.
	}

	return shader;
}

void loadDataFromFile( float** positionDataOut, int* nPositionsLoadedOut, int** triangleDataOut, int* nTrianglesLoadedOut )
{
    // There are a variety of 3d model file formats out there. For this example, we are using 
    // a simple .obj file created by an export from the open source Blender model creation
    // software. Unlike most file formats, .obj are plaintext rather than binary. This makes
    // them both slow to parse or load and makes them very easy to understand, modify by hand,
    // or write simple loading functions. For more details on the file being loaded, see the
    // comments in the .obj file itself. It also has an associated .mtl file, which is similarly
    // simple, though we don't actually use its contents for this example.

    std::vector< float > positionData;
    std::vector< int > triangleData;

    // Load the file...
    std::ifstream file("../tut_object.obj");

    // There are faster/better ways of loading models, but optimizing file IO is not the point of this.
    while( !file.eof() && file.good() )
    {
        std::string openingString;
        file >> openingString;

        // Vertex position data on this line; load them.
        if( openingString == "v" )
        {
            float newValue;

            // Each has 3 positions (xyz); load each.
            file >> newValue;
            positionData.push_back( newValue );
            file >> newValue;
            positionData.push_back( newValue );
            file >> newValue;
            positionData.push_back( newValue );
        }
        // Triangle 'index' data, telling which points to build triangles with
        else if( openingString == "f" )
        {
            int triangleIndex;

            // Each triangle obviously has 3 points.
            file >> triangleIndex;
            triangleData.push_back( triangleIndex-1 );    // Subtract 1, since this format seems to start at 1 rather than 0 for its indices.
            file >> triangleIndex;
            triangleData.push_back( triangleIndex-1 );
            file >> triangleIndex;
            triangleData.push_back( triangleIndex-1 );
        }
        // In this example, we don't care about any of the other data in the file. (This technically is a bad way to handle it, as any occurance of "f" or "v" would trigger an attempt to load data, even in comments. Fine in this case since there aren't any.)
        else
        {
        }

    }

    // 3 pieces of data per position
    *nPositionsLoadedOut = positionData.size() / 3;
    *positionDataOut = new float[positionData.size()];
    memcpy( *positionDataOut, &positionData[0], positionData.size() * sizeof( float ) );

    // 3 pieces of data per triangle
    *nTrianglesLoadedOut = triangleData.size() / 3;
    *triangleDataOut = new int[triangleData.size()];
    memcpy( *triangleDataOut, &triangleData[0], triangleData.size() * sizeof( int ) );
}

// Here we set up a pair of general-purpose buffers. These will be
// GL_TEXTURE_BUFFER type objects, which you can think of as a 1D 
// texture set up solely to act as a buffer. This allows us to access
// it with integer index values (like an array), as well as disabling the 
// 0.0 to 1.0 sampler methods of accessing it. There are a variety of
// types of textures/buffers you can send to the GPU -- I HIGHLY
// recommend reading up on them; some are fast, some are slow, and
// many have very strict, small size limits. 
void initBuffers()
{
    // Generate the buffers -- These are the internal data
    // structure we will be using to store our data in.
    GLuint buffers[2];
    glGenBuffers( 2, buffers );
    positionBuffer = buffers[0];
    triangleBuffer = buffers[1];

    // Generate the textures -- These are how we will access them
    // in our shaders. They will be linked to the buffers later.
    GLuint textures[2];
    glGenTextures( 2, textures );
    positionBufferTexture = textures[0];
    triangleBufferTexture = textures[1];

    // These will be initialized by the loading function.
    int nPositionsLoaded;
    float* positionData;
    int nTrianglesLoaded;
    int* triangleData;

    loadDataFromFile( &positionData, &nPositionsLoaded, &triangleData, &nTrianglesLoaded );
    numTriangles = nTrianglesLoaded;

    // Bind the buffers to GL_ARRAY_BUFFER and write our loaded data to it.
    glBindBuffer( GL_TEXTURE_BUFFER, positionBuffer );
    glBufferData( GL_TEXTURE_BUFFER, sizeof( float ) * 3 * nPositionsLoaded, positionData, GL_STATIC_DRAW );

    glBindBuffer( GL_TEXTURE_BUFFER, triangleBuffer );                                // By setting this one to the same 'target slot,' it implicitly un-binds the previous one.
    glBufferData( GL_TEXTURE_BUFFER, sizeof( int ) * 3 * nTrianglesLoaded, triangleData, GL_STATIC_DRAW );


    // Bind the textures to GL_TEXTURE_BUFFER, then link the buffers we created to them.
    // These textures are now ready to be bound to a texture slot and used in shaders.
    glBindTexture( GL_TEXTURE_BUFFER, positionBufferTexture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGB32F, positionBuffer );       // We need to retrieve 3 floats for a position, so store them as 3 channel

    glBindTexture( GL_TEXTURE_BUFFER, triangleBufferTexture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32I, triangleBuffer );         // We need to retrieve 1 int for a position index, so store them as 1 channel


    //To reiterate, for setting up texture-buffers:
    // 1. Generate Textures
    // 1. Generate Buffers
    // 2. Initialize data in Buffers.
    // 3. Link textures to buffers.

    // And later to send it to your shaders, bind it to a texture slot.

    // Encapsulating all these operations into some sort of TextureBuffer class would probably
    // be useful here to reduce the boilerplate code needed and reduce chance for errors.

    // And again, there are various types of buffers and textures; their initialization
    // patterns share some similarities, but be sure to read up on openGL docs online
    // to get the specifics. Some are very useful, others less so.
}

// Initialization code
void init()
{
	// Initializes the glew library
	glewInit();

	// Enables the depth test, which you will want in most cases. You can disable this in the render loop if you need to.
	glEnable(GL_DEPTH_TEST);
	

	// Read in the shader code from a file.
	std::string vertShader = readShader("VertexShader.glsl");
	std::string fragShader = readShader("FragmentShader.glsl");

	// createShader consolidates all of the shader compilation code
	vertex_shader = createShader(vertShader, GL_VERTEX_SHADER);
	fragment_shader = createShader(fragShader, GL_FRAGMENT_SHADER);

	// A shader is a program that runs on your GPU instead of your CPU. In this sense, OpenGL refers to your groups of shaders as "programs".
	// Using glCreateProgram creates a shader program and returns a GLuint reference to it.
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);		// This attaches our vertex shader to our program.
	glAttachShader(program, fragment_shader);	// This attaches our fragment shader to our program.

	// This links the program, using the vertex and fragment shaders to create executables to run on the GPU.
	glLinkProgram(program);
	// End of shader and program creation

	// This gets us a reference to the uniform variable in the vertex shader, which is called "MVP".
	// We're using this variable as a 4x4 transformation matrix
	// Only 2 parameters required: A reference to the shader program and the name of the uniform variable within the shader code.
	uniMVP = glGetUniformLocation(program, "MVP");

    // Initialize our buffers.
    initBuffers();

	// Creates the view matrix using glm::lookAt.
	// First parameter is camera position, second parameter is point to be centered on-screen, and the third paramter is the up axis.
	view = glm::lookAt(	glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Creates a projection matrix using glm::perspective.
	// First parameter is the vertical FoV (Field of View), second paramter is the aspect ratio, 3rd parameter is the near clipping plane, 4th parameter is the far clipping plane.
	proj = glm::perspective(45.0f, 800.0f / 600.0f, 0.1f, 1000.0f);

    trans = glm::translate( trans, glm::vec3( 0.0, 0.0, 0.0 ) );

	// Determines the interpretation of polygons for rasterization. The first parameter, face, determines which polygons the mode applies to.
	// The face can be either GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK
	// The mode determines how the polygons will be rasterized. GL_POINT will draw points at each vertex, GL_LINE will draw lines between the vertices, and 
	// GL_FILL will fill the area inside those lines.
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

int main(int argc, char **argv)
{
	// Initializes the GLFW library
	glfwInit();

	// Creates a window given (width, height, title, monitorPtr, windowPtr).
	// Don't worry about the last two, as they have to do with controlling which monitor to display on and having a reference to other windows. Leaving them as nullptr is fine.
	GLFWwindow* window = glfwCreateWindow(800, 600, "Oh look a heightmap!", nullptr, nullptr);

	// Makes the OpenGL context current for the created window.
	glfwMakeContextCurrent(window);

	// Sets the number of screen updates to wait before swapping the buffers.
	glfwSwapInterval(1);

	// Initializes most things needed before the main loop
	init();

	// Enter the main loop.
	while (!glfwWindowShouldClose(window))
	{
		// Call to the update function; should always be before rendering.
		update();

		// Call the render function.
		renderScene();

		// Swaps the back buffer to the front buffer
		// Remember, you're rendering to the back buffer, then once rendering is complete, you're moving the back buffer to the front so it can be displayed.
		glfwSwapBuffers(window);

		// Checks to see if any events are pending and then processes them.
		glfwPollEvents();
	}

	// After the program is over, cleanup your data!
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	glDeleteProgram(program);
	// Note: If at any point you stop using a "program" or shaders, you should free the data up then and there.


	// Frees up GLFW memory
	glfwTerminate();

	return 0;
}