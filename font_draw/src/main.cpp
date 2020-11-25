#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include "../../external/glad/glad.h"
#include <SDL2/SDL.h>

#include "../../lib/ogl/shader.h"
#include "../../lib/ogl/shaderbuffer.h"

#include <vector>
#include <filesystem>
#include <fstream>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

struct GPUVertexData
{
	float posX;
	float posY;
	uint16_t pixelSizeX;
	uint16_t pixelSizeY;
	uint32_t color;
};

static const char *vertSrc = R"(
	#version 450 core

	layout (location = 0) uniform vec2 windowSize;

	struct VData
	{
		vec2 vpos;
		uint vSizes;
		uint vColor;
	};

	layout (std430, binding=0) buffer shader_data
	{
		VData values[];
	};


	layout (location = 0) out vec4 colOut;
	void main()
	{
		int quadId = gl_VertexID / 4;
		int vertId = gl_VertexID % 4;

/*
		vec2 p = vec2(cos(posRotSize.z), sin(posRotSize.z));
		p = vec2(	p.x * aPos.x - p.y * aPos.y,
					p.y * aPos.x + p.x * aPos.y);
					*/

		
		vec2 p = vec2(-0.5f, -0.5f);
		p.x = (vertId + 1) % 4 < 2 ? -0.5f : 0.5f;
		p.y = vertId < 2 ? -0.5f : 0.5f;
		vec2 vSize = vec2(float(values[quadId].vSizes & 65535u), float((values[quadId].vSizes >> 16) & 65535u)); 
		p *= vSize;
		p += values[quadId].vpos;
		p /= windowSize * 0.5f;
		p -= 1.0f;
		p += 0.25f / windowSize;
		
		gl_Position = vec4(p.xy, 0.5, 1.0);
		vec4 c = vec4(0, 0, 0, 0);
		c.r = float((values[quadId].vColor >> 0) & 255) / 255.0f;
		c.g = float((values[quadId].vColor >> 8) & 255) / 255.0f;
		c.b = float((values[quadId].vColor >> 16) & 255) / 255.0f;
		c.a = float((values[quadId].vColor >> 24) & 255) / 255.0f;
		colOut = c;
	})";

static const char *fragSrc = R"(
	#version 450 core
	layout(origin_upper_left) in vec4 gl_FragCoord;
	layout (location = 0) out vec4 col;

	layout (location = 0) in vec4 colIn;
	layout(depth_unchanged) out float gl_FragDepth;
	
	void main()
	{
		col = colIn;
	})";


bool loadData(const std::string &fileName, std::vector<char> &dataOut)
{
	//
	if(std::filesystem::exists(fileName))
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

bool saveData(const std::string &fileName, const std::vector<char> &data)
{
	//
	if(std::filesystem::exists(fileName))
	{
		std::filesystem::path p(fileName);


		std::ofstream f(p, std::ios::out | std::ios::binary);


		f.write(data.data(), data.size());

		printf("filesize: %u\n", uint32_t(data.size()));
		return true;
	}
	return false;
}


static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam);


class App
{
public:
	bool init(const char *windowStr)
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
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);

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

	virtual ~App()
	{
		if(mainContext)
			SDL_GL_DeleteContext(mainContext);
		mainContext = nullptr;
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	void resizeWindow(int w, int h)
	{
		windowWidth = w;
		windowHeight = h;
		printf("Window size: %i: %i\n", w, h);
		glViewport(0, 0, w, h);
	}

	void setVsyncEnabled(bool enable)
	{
		vSync = enable;
		// Use v-sync
		SDL_GL_SetSwapInterval(vSync);
	}

	public: 
		SDL_Window *window = nullptr;
		SDL_GLContext mainContext = nullptr;		
		int windowWidth = 0;
		int windowHeight = 0;
		bool vSync = true;
};


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

static void mainProgramLoop(App &app, std::vector<char> &data, std::string &filename)
{
	Shader shader;
	if(!shader.initShader(vertSrc, fragSrc))
	{
		printf("Failed to init shader\n");
		return;
	}


	ShaderBuffer ssbo(GL_SHADER_STORAGE_BUFFER, 10240u * 16u, GL_DYNAMIC_COPY, nullptr);
	
	SDL_Event event;
	bool quit = false;
	float dt = 0.0f;
	float angle = 0.0f;

	Uint64 nowStamp = SDL_GetPerformanceCounter();
	Uint64 lastStamp = 0;
	double freq = (double)SDL_GetPerformanceFrequency();

	uint32_t chosenLetter = 'a';
	//uint32_t lastTicks = SDL_GetTicks();

	std::vector<uint32_t> indices;
	indices.resize(6 * 10240);
	for(int i = 0; i < 10240; ++i)
	{
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;

		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 2;
		indices[i * 6 + 5] = i * 4 + 3;
	}


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	ShaderBuffer indexBuffer(
		GL_ELEMENT_ARRAY_BUFFER, 
		indices.size() * sizeof(uint32_t), 
		GL_STATIC_DRAW,
		indices.data() 
		);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.handle)
;	//glEnableVertexAttribArray(0);  

	std::vector<GPUVertexData> vertData;
	vertData.resize(12*8* (128-32 + 1) + 1);


	static constexpr float buttonSize = 20.0f;
	static constexpr float smallButtonSize = 2.0f;
	static constexpr float borderSizes = 2.0f;

	{
		float offX = (borderSizes + buttonSize) + app.windowWidth * 0.5f;
		float offY = (borderSizes + buttonSize) + app.windowHeight * 0.5f;

		GPUVertexData &vdata = vertData[0];
		vdata.color = (255u) + (255u < 24u);
		vdata.pixelSizeX = smallButtonSize * 8 + 4;
		vdata.pixelSizeY = smallButtonSize * 12 + 4;
		vdata.posX = offX;
		vdata.posY = offY;
	}

	for(int j = 0; j < 12; ++j)
	{
		for(int i = 0; i < 8; ++i)
		{
			float offX = float((i - 4) * (borderSizes + buttonSize)) + app.windowWidth * 0.5f;
			float offY = float((j - 6) * (borderSizes + buttonSize)) + app.windowHeight * 0.5f;

			GPUVertexData &vdata = vertData[i + j * 8 + 1];
			vdata.color = 0;
			vdata.pixelSizeX = vdata.pixelSizeY = buttonSize;
			vdata.posX = offX;
			vdata.posY = offY;
		}
	}

	for(int k = 0; k < 128 - 32; ++k)
	{
		int x = k % 8;
		int y = k / 8;
		for(int j = 0; j < 12; ++j)
		{
			for(int i = 0; i < 8; ++i)
			{
				GPUVertexData &vdata = vertData[i + j * 8 + (k + 1) * 8 * 12 + 1];

				float smallOffX = float(i * (smallButtonSize)) + 10.0f + float(x * 8) * smallButtonSize + x * 2;
				float smallOffY = float(j * (smallButtonSize)) + 10.0f + float(y * 12) * smallButtonSize + y * 2;

				uint32_t indx = k * 12 + j;
				bool isVisible = ((data[indx] >> i) & 1) == 1;

				vdata.color = isVisible ? ~0u : 0u;
				vdata.pixelSizeX = vdata.pixelSizeY = smallButtonSize;
				vdata.posX = smallOffX;
				vdata.posY = smallOffY;

			}
		}
	}

	while (!quit)
	{
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = float((nowStamp - lastStamp)*1000 / freq );


		int mouseX, mouseY;
		uint32_t mousePress = SDL_GetMouseState(&mouseX, &mouseY);

		mouseY = app.windowHeight - mouseY;

		bool mouseLeftDown = (mousePress & SDL_BUTTON(SDL_BUTTON_LEFT)) == SDL_BUTTON(SDL_BUTTON_LEFT);
		bool mouseRightDown = (mousePress & SDL_BUTTON(SDL_BUTTON_RIGHT)) == SDL_BUTTON(SDL_BUTTON_RIGHT);

		angle += 0.001f * dt;
		shader.useProgram();

		glUniform2f(0, app.windowWidth, app.windowHeight);

		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;
				
				case SDL_KEYDOWN:
				{
					if(((event.key.keysym.mod) & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_s)
					{
						// save;
						saveData(filename, data);
					}
					else if((event.key.keysym.mod & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_l)
					{
						// load;
						loadData(filename, data);
					}

					else if(event.key.keysym.sym >= SDLK_SPACE && 
						event.key.keysym.sym < 128)
					{
						chosenLetter = event.key.keysym.sym;
						//updateArray(arr, data, chosenLetter);
					}
					else switch(event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							quit = true;
							break;

						case SDLK_RIGHT:
							if(chosenLetter < 127)
								++chosenLetter;
							break; 

						case SDLK_LEFT:
							if(chosenLetter > 32)
								--chosenLetter;
							break;

						case SDLK_UP:
							chosenLetter += 8;
							if(chosenLetter > 127)
								chosenLetter = 127;
							break;

						case SDLK_DOWN:
							chosenLetter -= 8;
							if(chosenLetter < 32)
								chosenLetter = 32;
							break;

						default:
							break;
					}
				}
				 case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						app.resizeWindow(event.window.data1, event.window.data2);
						glUniform2f(0, app.windowWidth, app.windowHeight);
					}
					break;
			}
		}

		 //Clear color buffer
		glClear( GL_COLOR_BUFFER_BIT );
		
		shader.useProgram();


		
		for(int j = 0; j < 12; ++j)
		{
			
			for(int i = 0; i < 8; ++i)
			{
				float offX = float((i - 4) * (borderSizes + buttonSize)) + app.windowWidth * 0.5f;
				float offY = float((j - 6) * (borderSizes + buttonSize)) + app.windowHeight * 0.5f;

				bool insideRect = mouseX > offX - (borderSizes + buttonSize) * 0.5f &&
					mouseX < offX + (borderSizes + buttonSize) * 0.5f &&
					mouseY > offY - (borderSizes + buttonSize) * 0.5f &&
					mouseY < offY + (borderSizes + buttonSize) * 0.5f;

				uint32_t indx = (chosenLetter - 32) * 12 + j;

				if(mouseLeftDown && insideRect)
					data[indx] |= (1 << i);
				else if(mouseRightDown && insideRect)
					data[indx] &= ~(char(1 << i));
				
				bool isVisible = ((data[indx] >> i) & 1) == 1;

				vertData[i + j * 8 + 1].color = isVisible ? ~0u : 0u;
				vertData[(indx + 12) * 8 + i + 1].color = isVisible ? ~0u : 0u;

			}

		}
		uint32_t xOff = (chosenLetter - 32) % 8;
		uint32_t yOff = (chosenLetter - 32) / 8;
		
		vertData[0].posX = 10.0f + (4 + xOff * 8) * smallButtonSize + xOff * 2 - 1;
		vertData[0].posY = 10.0f + (6 + yOff * 12) * smallButtonSize + yOff * 2 - 1;


		ssbo.updateBuffer(0, vertData.size() * sizeof(GPUVertexData), vertData.data());
		ssbo.bind(0);
//		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(VAO);
		
		glDrawElements(GL_TRIANGLES, vertData.size() * 6, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(app.window);
		SDL_Delay(1);

		char str[100];
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, mbs: %i ml: %i, mr: %i, Letter: %c", 
			dt, 1000.0f / dt, mouseX, mouseY, mousePress, mouseLeftDown, mouseRightDown, char(chosenLetter));
		SDL_SetWindowTitle(app.window, str);

		//printf("Frame duration: %f fps: %f\n", dt, 1000.0f / dt);
	}
}

int main(int argCount, char **argv) 
{
	std::vector<char> data;
	std::string filename;
	if(argCount < 2)
	{
		filename = "new_font.dat";
	}
	else
	{
		filename = argv[1];
	}
	
	loadData(filename, data);
	App app;
	if(app.init("OpenGL 4.5"))
	{
		mainProgramLoop(app, data, filename);
	}

	return 0;
}