#include "third_party/glad/include/glad/glad.h"
#include "third_party/glad/include/glad/glad_wgl.h"
#define GLFW_EXPOSE_NATIVE_WIN32 1
#define GLFW_EXPOSE_NATIVE_WGL 1
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <windows.h>

#include "GrContext.h"
//#include "third_party/skia/src/gpu/gl/GrGLUtil.h"

#include "SkCanvas.h"
#include "SkImage.h"
#include "SkRSXform.h"
#include "SkSurface.h"
#include "GrGLInterface.h"

GrContext* sContext = nullptr;
SkSurface* sSurface = nullptr;
GLuint fbo;

#include <iostream>
static void init_skia(int w, int h) {
	//glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	// Create the native interface.
	auto backend_context =
		reinterpret_cast<GrBackendContext>(GrGLCreateNativeInterface());
	// setup GrContext

	sk_sp<const GrGLInterface> interface(GrGLCreateNativeInterface());

	// To use NVPR, comment this out
	SkASSERT(interface);
	sContext = GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)interface.get());

	SkImageInfo info = SkImageInfo::MakeN32Premul(960, 640);
	GrBackendRenderTargetDesc desc;
	desc.fWidth = w;
	desc.fHeight = h;
	desc.fConfig = kSkia8888_GrPixelConfig;
	desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
	desc.fSampleCnt = 1;
	desc.fStencilBits = 8;
	//GR_GL_GetIntegerv(backend_context, GR_GL_FRAMEBUFFER_BINDING, &buffer);
	//glGenFramebuffers(1, &fbo);
	desc.fRenderTargetHandle = 0;  // assume default framebuffer
	sSurface = SkSurface::MakeRenderTarget(sContext, SkBudgeted::kNo, info).release();// desc, nullptr, nullptr).release();
	//sContext->resetContext();
	//glPopClientAttrib();
}

static void cleanup_skia() {
	delete sSurface;
	delete sContext;
}

const int kGrid = 100;
const int kWidth = 960;
const int kHeight = 640;
// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Shaders
const GLchar* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 position;\n"
"void main()\n"
"{\n"
"gl_Position = vec4(position.x, position.y, position.z, 1.0);\n"
"}\0";
const GLchar* fragmentShaderSource = "#version 330 core\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";

void maint() {
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	static const int kStencilBits = 8;  // Skia needs 8 stencil bits

	glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);
	glfwWindowHint(GLFW_RED_BITS, kStencilBits);
	glfwWindowHint(GLFW_GREEN_BITS, kStencilBits);
	glfwWindowHint(GLFW_BLUE_BITS, kStencilBits);
	glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	glfwWindowHint(GLFW_DEPTH_BITS, 1);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	HWND desktop = GetDesktopWindow();
	HDC ourWindowHandleToDeviceContext = GetDC(desktop);

	//gladLoadWGL(ourWindowHandleToDeviceContext);
	int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		0, 0
	};
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
		(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	auto _context = wglCreateContextAttribsARB(ourWindowHandleToDeviceContext, glfwGetWGLContext(window), context_attribs);

	wglMakeCurrent(ourWindowHandleToDeviceContext, _context);

}

// The MAIN function, from here we start the application and run the game loop
int main()
{

	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	static const int kStencilBits = 8;  // Skia needs 8 stencil bits

	glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);
	glfwWindowHint(GLFW_RED_BITS, kStencilBits);
	glfwWindowHint(GLFW_GREEN_BITS, kStencilBits);
	glfwWindowHint(GLFW_BLUE_BITS, kStencilBits);
	glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	glfwWindowHint(GLFW_DEPTH_BITS, 1);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	//PIXELFORMATDESCRIPTOR kPixelFormatDescriptor;
	//HWND desktop = GetDesktopWindow();
	//HDC ourWindowHandleToDeviceContext = GetDC(desktop);
	//DescribePixelFormat(ourWindowHandleToDeviceContext, 1, sizeof(PIXELFORMATDESCRIPTOR), &kPixelFormatDescriptor);
	//SetPixelFormat(ourWindowHandleToDeviceContext,
	//	1,
	//	&kPixelFormatDescriptor);

	//gladLoadWGL(ourWindowHandleToDeviceContext);
	//int context_attribs[] = {
	//	WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
	//	WGL_CONTEXT_MINOR_VERSION_ARB, 1,
	//	WGL_STENCIL_BITS_ARB, 8,
	//	WGL_RED_BITS_ARB, 8,
	//	WGL_GREEN_BITS_ARB, 8,
	//	WGL_BLUE_BITS_ARB, 8,
	//	WGL_DOUBLE_BUFFER_ARB, 1,
	//	WGL_DEPTH_BITS_ARB, 1,
	//	0,
	//};
	//const int attribs[] = {
	//	WGL_CONTEXT_MAJOR_VERSION_ARB,
	//	3,
	//	WGL_CONTEXT_MINOR_VERSION_ARB,
	//	1,
	//	0,
	//	0,
	//};
	//auto _context = wglCreateContextAttribsARB(ourWindowHandleToDeviceContext, 0, attribs);
	//auto _context = wglCreateContextAttribsARB(GetDC(glfwGetWin32Window(window)), 0, attribs);


	//glfwMakeContextCurrent(window);
	//wglMakeCurrent(ourWindowHandleToDeviceContext, _context);
	init_skia(kWidth, kHeight);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	//gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
	// Define the viewport dimensions
	int width, height;
	//glfwGetFramebufferSize(window, &width, &height);
	//glViewport(0, 0, width, height);


	//// Build and compile our shader program
	//// Vertex shader
	//GLint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//glCompileShader(vertexShader);
	//// Check for compile time errors
	//GLint success;
	//GLchar infoLog[512];
	//glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	//if (!success)
	//{
	//	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	//}
	//// Fragment shader
	//GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//glCompileShader(fragmentShader);
	//// Check for compile time errors
	//glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	//if (!success)
	//{
	//	glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	//}
	//// Link shaders
	//GLint shaderProgram = glCreateProgram();
	//glAttachShader(shaderProgram, vertexShader);
	//glAttachShader(shaderProgram, fragmentShader);
	//glLinkProgram(shaderProgram);
	//// Check for linking errors
	//glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	//if (!success) {
	//	glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	//}
	//glDeleteShader(vertexShader);
	//glDeleteShader(fragmentShader);


	// Set up vertex data (and buffer(s)) and attribute pointers
	//GLfloat vertices[] = {
	//  // First triangle
	//   0.5f,  0.5f,  // Top Right
	//   0.5f, -0.5f,  // Bottom Right
	//  -0.5f,  0.5f,  // Top Left 
	//  // Second triangle
	//   0.5f, -0.5f,  // Bottom Right
	//  -0.5f, -0.5f,  // Bottom Left
	//  -0.5f,  0.5f   // Top Left
	//}; 
	GLfloat vertices[] = {
		0.5f,  0.5f, 0.0f,  // Top Right
		0.5f, -0.5f, 0.0f,  // Bottom Right
		-0.5f, -0.5f, 0.0f,  // Bottom Left
		-0.5f,  0.5f, 0.0f   // Top Left 
	};
	GLuint indices[] = {  // Note that we start from 0!
		0, 1, 3,  // First Triangle
		1, 2, 3   // Second Triangle
	};
	GLuint VBO, VAO, EBO;
	//glGenVertexArrays(1, &VAO);
	//glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	//glBindVertexArray(VAO);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	//glEnableVertexAttribArray(0);

	//glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	//glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO


	//											// Uncommenting this call will result in wireframe polygons.
	//											//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//											// Game loop
	//											// Draw to the surface via its SkCanvas.
	////SKIA STUFF
	//glViewport(0, 0, WIDTH, HEIGHT);
	//glClearColor(1, 1, 1, 1);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	SkCanvas* canvas = sSurface->getCanvas(); // We don't manage this pointer's lifetime.
	SkPaint paint;
	paint.setFilterQuality(kLow_SkFilterQuality);
	paint.setColor(SK_ColorWHITE);
	paint.setTextSize(15.0f);
	//glViewport(0, 0, WIDTH, HEIGHT);
	//glClearColor(0, 0, 0, 1);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	while (!glfwWindowShouldClose(window))
	//while (true)
	{
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
		//glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
		//glUseProgram(shaderProgram);
		//glBindVertexArray(VAO);
		//glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		//glBindVertexArray(0);
		//glUseProgram(0);

		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//canvas->discard();
		sContext->resetContext();
		canvas->clear(SK_ColorTRANSPARENT);
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
		canvas->drawText("hello World!", strlen("hello World!"), 200.f, 200.f, paint);
		canvas->drawRect(SkRect::MakeXYWH(10.f, 100.f, 100.f, 100.f), paint);
		canvas->drawCircle(50.f, 50.f, 50.f, paint);
		sk_sp<SkImage> image = sSurface->makeImageSnapshot();
		if (!image || !image->isTextureBacked()) {
			printf("No Image\n");
		}
		sk_sm<SkData> data = image->encode();
		//glDisable(GL_STENCIL_TEST);
		//glUseProgram(shaderProgram);
		//glBindVertexArray(VAO);
		//glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		//glBindVertexArray(0);
		//glUseProgram(0);

		//glPopClientAttrib();

		// Draw our first triangle

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}