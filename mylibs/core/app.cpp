#include "app.h"
#include <SDL2/SDL.h>

#include "glad/glad.h"

#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cmath>

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
	
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		fprintf(stderr, "%s\n", message);
	}

	if (severity==GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "Aborting...\n");
		abort();
	}
}


namespace core {

bool App::init(const char *windowStr, int screenWidth, int screenHeight)
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

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	// Debug!
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);


	window = SDL_CreateWindow(windowStr, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screenWidth, screenHeight, SDL_WINDOW_OPENGL);


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

/*
	// Enable the debug callback
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
*/

	// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	
	resizeWindow(w, h);
	glClearColor(0.0f, 0.5f, 1.0f, 0.0f);
	printf("Screen res: %i:%i\n", w, h);

	SDL_SetWindowResizable(window, SDL_TRUE);

	// rdoc....
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	//glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

	return true;

}

App::~App()
{
	if(mainContext)
		SDL_GL_DeleteContext(mainContext);
	mainContext = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;
}

void App::resizeWindow(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	printf("Window size: %i: %i\n", w, h);
	glViewport(0, 0, w, h);
}

void App::setVsyncEnabled(bool enable)
{
	vSync = enable;
	// Use v-sync
	SDL_GL_SetSwapInterval(vSync);
}



void App::setClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}


bool loadFontData(const std::string &fileName, std::vector<char> &dataOut)
{
	//
	if (std::filesystem::exists(fileName))
	{
		std::filesystem::path p(fileName);
		uint32_t s = uint32_t(std::filesystem::file_size(p));

		dataOut.resize(s);

		std::ifstream f(p, std::ios::in | std::ios::binary);


		f.read(dataOut.data(), s);

		printf("filesize: %u\n", s);
		return true;
	}
	return false;
}


// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a)
{
	r = fmaxf(0.0f, fminf(r, 1.0f));
	g = fmaxf(0.0f, fminf(g, 1.0f));
	b = fmaxf(0.0f, fminf(b, 1.0f));
	a = fmaxf(0.0f, fminf(a, 1.0f));

	uint32_t c = 0u;
	c += (uint32_t(r * 255.0f) & 255u);
	c += (uint32_t(g * 255.0f) & 255u) << 8u;
	c += (uint32_t(b * 255.0f) & 255u) << 16u;
	c += (uint32_t(a * 255.0f) & 255u) << 24u;
	
	return c;
}

}; // end of core namespace.
