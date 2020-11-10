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

static int windowSizeX = 0;
static int windowSizeY = 0;

static const int vsync = 1;

static constexpr char *vertSrc = R"(
	#version 450 core
	layout (location = 0) in vec3 aPos;

	layout (location = 0) uniform vec4 posRotSize;
	layout (location = 1) uniform vec2 windowSize;
	layout (location = 2) uniform vec3 colIn;

	layout (location = 0) out vec3 col;
	void main()
	{
		vec2 p = vec2(cos(posRotSize.z), sin(posRotSize.z));
		p = vec2(	p.x * aPos.x - p.y * aPos.y,
					p.y * aPos.x + p.x * aPos.y);
		p *= posRotSize.w;
		p += posRotSize.xy;
		p /= windowSize * 0.5f;
		p -= 1.0f;
	   gl_Position = vec4(p.xy, aPos.z, 1.0);
	   col = colIn;
	})";

static const char *fragSrc = R"(
	#version 450 core
	layout (location = 0) out vec4 col;

	layout (location = 0) in vec3 colIn;

	void main()
	{
		col = vec4(colIn, 1.0f);
	})";


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
	windowSizeX = w;
	windowSizeY = h;
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

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

	glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

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
		0.5f, 0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,

		-0.5f,  -0.5f, 0.0f,
		0.5f,  -0.5f, 0.0f,
		0.5f,  0.5f, 0.0f

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

	bool arr[8*12] = {};

	//uint32_t lastTicks = SDL_GetTicks();
	while (!quit)
	{
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = float((nowStamp - lastStamp)*1000 / freq );


		int mouseX, mouseY;
		uint32_t mousePress = SDL_GetMouseState(&mouseX, &mouseY);
		bool mouseLeftDown = (mousePress & SDL_BUTTON(SDL_BUTTON_LEFT)) == SDL_BUTTON(SDL_BUTTON_LEFT);
		bool mouseRightDown = (mousePress & SDL_BUTTON(SDL_BUTTON_RIGHT)) == SDL_BUTTON(SDL_BUTTON_RIGHT);

		angle += 0.001f * dt;
		shader.useProgram();
		glUniform4f(0, float(windowSizeX * -0.5f) + float(mouseX), 
			float(windowSizeY * -0.5f) + float(mouseY), angle, 100.0f);
		glUniform2f(1, windowSizeX, windowSizeY);

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
						case SDLK_c:
						{
							for(int i = 0; i < 12 * 8; ++i)
								arr[i] = false;
						}
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
		static constexpr float buttonSize = 20.0f;
		static constexpr float smallButtonSize = 2.0f;
		static constexpr float borderSizes = 2.0f;
		for(int j = 0; j < 12; ++j)
		{
			
			for(int i = 0; i < 8; ++i)
			{
				float offX = float((i - 4) * (borderSizes + buttonSize)) + windowSizeX * 0.5f;
				float offY = float((j - 6) * (borderSizes + buttonSize)) + windowSizeY * 0.5f;

				float smallOffX = float(i * (smallButtonSize)) + 10.0f;
				float smallOffY = float(j * (smallButtonSize)) + 10.0f;

				bool insideRect = mouseX > offX - (borderSizes + buttonSize) * 0.5f &&
					mouseX < offX + (borderSizes + buttonSize) * 0.5f &&
					mouseY > offY - (borderSizes + buttonSize) * 0.5f &&
					mouseY < offY + (borderSizes + buttonSize) * 0.5f;
				if(mouseLeftDown && insideRect)
					arr[i + j * 8] = true;
				else if(mouseRightDown && insideRect)
					arr[i + j * 8] = false;
				


				if(arr[i + j * 8])
					glUniform3f(2, 1.0f, 1.0f, 1.0f);
				else
					glUniform3f(2, 0.0f, 0.0f, 0.0f);
				glUniform4f(0, offX, offY, 0.0f, buttonSize);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				if(arr[i + j * 8])
				{
					glUniform4f(0, smallOffX, smallOffY, 0.0f, smallButtonSize);
					glDrawArrays(GL_TRIANGLES, 0, 6);

				}
			}
		}
		SDL_Delay(1);
		
		SDL_GL_SwapWindow(window);

		char str[100];
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, mbs: %i ml: %i, mr: %i", dt, 1000.0f / dt, mouseX, mouseY, mousePress, mouseLeftDown, mouseRightDown);
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