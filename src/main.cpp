#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include "glad.h"
#include <SDL2/SDL.h>

#include "shader.h"

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;
static SDL_Window *window = nullptr;
static SDL_GLContext mainContext;

static const int vsync = 1;

static const char *vertSrc = "#version 450 core\n"
    "layout (location = 0) in vec3 aPos;\n"
	"layout (location = 0) uniform float rotation;\n"

    "void main()\n"
    "{\n"
	"	vec2 p = vec2(cos(rotation), sin(rotation));\n"
	"	p = vec2(	p.x * aPos.x - p.y * aPos.y,\n"
	"				p.y * aPos.x + p.x * aPos.y);\n"
    "   gl_Position = vec4(p.x, p.y, aPos.z, 1.0);\n"
    "}\0";

static const char *fragSrc = "#version 450 core\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
	"	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
	"}\0";


static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	(void)source; (void)type; (void)id; 
	(void)severity; (void)length; (void)userParam;
	fprintf(stderr, "%s\n", message);
	if (severity==GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "Aborting...\n");
		abort();
	}
}
static void windowResized(int w, int h)
{
	printf("Window size: %i: %i\n", w, h);

	glViewport(0, 0, w, h);
}

static bool initGL(const char *windowStr)
{
	// Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL\n");
		return false;
	}
	atexit (SDL_Quit);
	SDL_GL_LoadLibrary(NULL); // Default OpenGL is fine.

	// Request an OpenGL 4.5 context (should be core)
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Debug!
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);


	window = SDL_CreateWindow(windowStr, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);


	if (window == NULL)
	{
		printf("Couldn't set video mode\n");
		return false;
	}
	
	mainContext = SDL_GL_CreateContext(window);
	if (mainContext == nullptr)
	{
		printf("Failed to create OpenGL context\n");
		return false;
	}


	// Check OpenGL properties
	printf("OpenGL loaded\n");
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	// Enable the debug callback
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

	// Use v-sync
	SDL_GL_SetSwapInterval(vsync);

	// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	windowResized(w, h);
	glClearColor(0.0f, 0.5f, 1.0f, 0.0f);
	printf("Screen res: %i:%i\n", w, h);

	SDL_SetWindowResizable(window, SDL_TRUE);

	return true;
}

static void mainProgramLoop()
{
	Shader shader;
	if(!shader.initShader(vertSrc, fragSrc))
	{
		printf("Failed to init shader\n");
		return;
	}

	float vertices[] = 
	{
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f,  0.5f, 0.0f
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);  

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);


	// ..:: Initialization code (done once (unless your object frequently changes)) :: ..
	// 1. bind Vertex Array Object
	glBindVertexArray(VAO);
	
	// 2. copy our vertices array in a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// 3. then set our vertex attributes pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);  


	
	
	SDL_Event event;
	bool quit = false;
	float dt = 0.0f;
	float angle = 0.0f;

	Uint64 nowStamp = SDL_GetPerformanceCounter();
	Uint64 lastStamp = 0;
	double freq = (double)SDL_GetPerformanceFrequency();

	//uint32_t lastTicks = SDL_GetTicks();
	while (!quit)
	{
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = float((nowStamp - lastStamp)*1000 / freq );

		angle += 0.001f * dt;
		shader.useProgram();
		glUniform1f(0, angle);

		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;
				
				case SDL_KEYDOWN:
				{
					switch(event.key.keysym.sym)
					{
						case SDLK_q:
						case SDLK_ESCAPE:
							quit = true;
							break;
						default:
							break;
					}
				}
				 case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						windowResized(event.window.data1, event.window.data2);
					}
					break;
			}
		}
		 //Clear color buffer
		glClear( GL_COLOR_BUFFER_BIT );
		
		shader.useProgram();

		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		SDL_Delay(1);
		
		SDL_GL_SwapWindow(window);

		char str[50];
		sprintf(str, "%2.2fms, fps: %4.2f", dt, 1000.0f / dt);
		SDL_SetWindowTitle(window, str);

		//printf("Frame duration: %f fps: %f\n", dt, 1000.0f / dt);
	}
}

int main() 
{
	if(initGL("OpenGL 4.5"))
	{
		mainProgramLoop();
		SDL_GL_DeleteContext(mainContext);
	}
}