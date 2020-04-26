#include <sys/stat.h>

//Types

struct MainConfig {
	uint32_t desiredFps;
	uint32_t scaling;
	bool fullscreen;
	bool showBorder;
	Sint32 woverride, hoverride;
	int h, w;
	
	bool hasAutoFile;
	string autoFile;
	bool hasAutoAudioFile;
	string autoAudioFile;

	bool audioEnabled;
	SeekMode seek;

	bool quitOnEof;
	bool useUI;
	bool useKeys;
	bool autostart;

	MainConfig() {
		desiredFps = 30;
		scaling = 4;
		autostart = quitOnEof = fullscreen = hasAutoFile = hasAutoAudioFile = showBorder = false;
		useUI = useKeys = audioEnabled = true;

		woverride = hoverride = -1;

		h = w = -1;

		//seek = S_DIRECT;
		seek = SeekMode::SEEK_ENHANCED;
	}
};

enum TransportState {
	T_NOCHANGE = -1,
	T_STOP = 1,
	T_PLAY,
	T_PAUSE
};

struct MainRunState {
	bool keeprunning;
	SDL_Event event;
	
	bool cdgLoaded;
	string cdgFile;
	ifstream cdgIn;
	struct stat cdgStat;
	int cdgLoc;

	bool audioLoaded;
	string audioFile;
	Mix_Music *audio;

	TransportState transport;
	TransportState nextTransport;
	
	uint8_t channel;
	uint8_t nextChannel;

	double nextLoc;

	bool dumpImage;

	MainRunState() {
		keeprunning = true;
		
		cdgLoaded = false;
		audioLoaded = false;

		audio = NULL;

		transport = T_STOP;
		nextTransport = T_NOCHANGE;
		
		channel = 1;
		nextChannel = 1;

		nextLoc = -1;

		dumpImage = false;
	}
};

extern int open_filechooser(MainRunState*, bool music = false);
