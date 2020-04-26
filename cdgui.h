/*UI design

[PLAY][PAUSE][STOP][SEEK BAR][LOAD CDG][LOAD AUDIO]

*/

#define UI_NUM_BUTTONS 6
#define UI_HEIGHT 24
#define UI_WIDTH (24*2)

enum Button {
	B_NONE = -1,
	B_PLAY = 1,
	B_LOADCDG,
	B_LOADAUDIO,
	B_SAVEIMAGE,
	B_PAUSE,
	B_STOP,
	B_SEEKBAR = 100
};

class CDGUI {

private:
	bool btnPlayDown, btnLoadCDGDown, btnLoadAudioDown, btnSaveImageDown, btnPauseDown, btnStopDown, btnSeekbarDown;
	TTF_Font *font;
	SDL_RWops *_fontMem;

	void resetButtonStates();
	Button mouseLocationToButton(const SDL_Event*, const MainConfig*);
public:
	CDGUI();
	~CDGUI();
	
	void processMouseDown(MainRunState*, const MainConfig*);
	void processMouseUp(MainRunState*, const MainConfig*);
	void render(SDL_Renderer*, const MainRunState*, const MainConfig*);

};