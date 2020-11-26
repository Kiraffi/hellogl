
struct SDL_Window;
typedef void *SDL_GLContext;

namespace core
{


class App
{
public:
	bool init(const char *windowStr, int screenWidth, int screenHeight);
	virtual ~App();
	void resizeWindow(int w, int h);
	void setVsyncEnabled(bool enable);

	public: 
		SDL_Window *window = nullptr;
		SDL_GLContext mainContext = nullptr;		
		int windowWidth = 0;
		int windowHeight = 0;
		bool vSync = true;
};



};