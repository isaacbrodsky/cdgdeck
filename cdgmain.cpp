// cdgmain.cpp
// Isaac Brodsky
// 2012 AUGUST 6

#include <cstring>
#include "stdafx.h"
#include "cdg.h"
#include "cdgmain.h"
#include "cdgui.h"
#include "cdgrenderer.h"
#include <FL/Fl.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>

#include <sys/stat.h>

#include "gen.h"

#ifdef WIN32
// Only available in POSIX
#define strcasecmp _stricmp
#endif

//Defines
#define DEFAULT_WIDTH CDG_WIDTH
#define DEFAULT_HEIGHT CDG_HEIGHT
#define DEFAULT_BPP 24

const char *DEFAULT_CAPTION = "CD+G Deck";
const char *COPYRIGHT_MESSAGE = "CD+G Deck v1.0.4\n"
	"Copyright 2012 Isaac Brodsky. All rights reserved.\n"
	"This program is provided with no warranty.\n"
	"\n"
	"Uses SDL under the zlib license,\n\tCopyright (C) 1997-2009 Sam Lantinga\n"
	"Uses SDL_ttf under the zlib license,\n\tCopyright (C) 1997-2009 Sam Lantinga\n"
	"Uses SDL_mixer under the zlib license,\n\tCopyright (C) 1997-2012 Sam Lantinga\n"
	"Uses libpng,\n\tlibpng zlib 1.5.13 - September 27, 2012\n"
    "\tCopyright (c) 1998-2012 Glenn Randers-Pehrson\n"
    "\tCopyright (c) 1996-1997 Andreas Dilger\n"
    "\tCopyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.\n"
	"Uses zlib,\n\tCopyright (C) 1995-2012 Jean-loup Gailly and Mark Adler\n"
	"Uses FLTK under the LGPL license,\n\tCopyright 1998-2010 by Bill Spitzak and others.\n"
	"Uses Bitstream Vera,\n"
	"\tCopyright (c) 2003 by Bitstream, Inc. All Rights Reserved.\n"
	"\tBitstream Vera is a trademark of Bitstream, Inc.\n"
	"\n";

//Prototypes
//these functions shouldn't be used outside of cdgmain.cpp
void process_save_cdg(const CDG & cdg, bool maskBorder);
void process_input(MainRunState *run, const MainConfig *config, CDGUI *ui);
void process_render(SDL_Renderer *renderer, int &dirty, const CDG &cdg, MainRunState *run, const MainConfig *config, CDGUI *ui);

SDL_Window *graphics_init(MainConfig*);
void process_args(int argc, char* argv[], MainConfig*);

//Functions
#if (WIN32 && NDEBUG)
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	//insane hack workaround provided by Microsoft (through msvcrt, I believe)
	//to retrieve properly parsed command line arguments.
	//These variables are automagically populated at runtime.
	//http://support.microsoft.com/kb/126571
	char **argv = __argv;
	int argc = __argc;
	
	//TODO
	//Probably would be a good idea to convert cerr << errs to use a function where
	//I can have an #if and pop up a messagebox on (WIN32 && NDEBUG)
#else
int main( int argc, char* argv[] ) {
#endif

	CDGUI *ui;

	try
	{
		ui = new CDGUI();
	}
	catch (exception &e)
	{
		cout << e.what() << endl;
		return 1000;
	}

	MainConfig config;
	MainRunState run;
	CDG *cdg = new CDG;
	int dirty = 0;
	SDL_Window* screen;
	SDL_Renderer* renderer;
	string caption = DEFAULT_CAPTION;
	
	cout << DEFAULT_CAPTION << endl
	     << "Copyright 2012 Isaac Brodsky" << endl
		 << "Press F1 or use -help to see full copyright message." << endl << endl;

	process_args(argc, argv, &config);

	screen = graphics_init(&config);
	if (screen == NULL) {
		return 1;
	}
	renderer = SDL_CreateRenderer(screen, -1, 0);
	
	// TODO: Replace SDL_mixer with something supporting seek
	if (Mix_Init(MIX_INIT_FLAC | MIX_INIT_MP3 | MIX_INIT_OGG) == 0) {
		// TODO: Fails on no OGG
		cerr << "audio open " << Mix_GetError() << endl;
		// config.audioEnabled = false;
	} else {
		atexit(Mix_Quit);
	}

	if (config.hasAutoFile) {
		run.cdgFile = config.autoFile;
		run.cdgLoaded = true;
		if (config.hasAutoAudioFile) {
			run.audioFile = config.autoAudioFile;
			run.audioLoaded = true;
		}
		stat(run.cdgFile.c_str(), &run.cdgStat);
		run.cdgLoc = 0;
		if (config.autostart)
			run.nextTransport = T_PLAY;
	}/* else {
		open_filechooser(&run);
		run.nextTransport = T_PLAY;
	}*/

	cout << "\n";

	uint32_t ticktime = SDL_GetTicks(), tickdeltatime, starttime, elapsedtime, loopbegintime, adjustmenttime = 0;
	
	//Keep track of the current frame
	uint32_t frame = 0, ticksnow, tickstotal = 0;

	while (run.keeprunning) {
		starttime = SDL_GetTicks();

		process_input(&run, &config, ui);

		if (run.cdgLoaded) {
			switch (run.nextTransport) {
			case T_PLAY:
				if (run.transport == T_STOP) {
					loopbegintime = SDL_GetTicks();
					adjustmenttime = 0;
					frame = 0;
					tickstotal = 0;

					stat(run.cdgFile.c_str(), &run.cdgStat);
					run.cdgLoc = 0;
					run.cdgIn.open(run.cdgFile, ios::binary);
					delete cdg;
					cdg = new CDG;

					//Set the window caption
					caption = DEFAULT_CAPTION;
					caption.append(" - ");
					caption.append(run.cdgFile);
					SDL_SetWindowTitle(screen, caption.c_str());
					
					dirty = 1;

					if (config.audioEnabled && run.audioLoaded) {
						if (run.audio != NULL) {
							Mix_FreeMusic(run.audio);
						}
						run.audio = Mix_LoadMUS(run.audioFile.c_str());
						if (!run.audio) {
							cerr << "start load mix: " << Mix_GetError() << endl;
						} else {
							if (Mix_PlayMusic(run.audio, 0) == -1) {
								cerr << "start mix: " << Mix_GetError() << endl;
							}
						}
					}
				} else {
					if (config.audioEnabled && run.audioLoaded) {
						Mix_ResumeMusic();
					}
				}
				break;
			case T_PAUSE:
				if (config.audioEnabled && run.audioLoaded) {
					Mix_PauseMusic();
				}
				break;
			case T_STOP:
				run.cdgIn.close();
				run.cdgLoc = 0;
				if (config.audioEnabled && Mix_PlayingMusic()) {
					Mix_HaltMusic();
					Mix_FreeMusic(run.audio);
					run.audio = NULL;
				}
				break;
			case T_NOCHANGE:
				// Do nothing.
				break;
			}
			if (run.nextTransport != T_NOCHANGE)
				run.transport = run.nextTransport;
			if (run.transport == T_PLAY && run.nextLoc != -1) {
				//keep playing music through this (as opposed to pausing and resuming
				//when ready... although that sounds completely reasonable too. :/
				if (config.audioEnabled && run.audioLoaded) {
					Mix_RewindMusic();
					Mix_PauseMusic();
				}
				run.cdgLoc = CDG::percentToSecond(run.nextLoc, run.cdgStat.st_size);
				cdg->seekTo(run.cdgIn, run.cdgLoc, config.seek);
				dirty = 1;
				if (config.audioEnabled && run.audioLoaded) {
					Mix_SetMusicPosition(run.cdgLoc / BYTES_PER_SECOND);
					Mix_ResumeMusic();
				}

				run.nextLoc = -1;
			}
			if (run.dumpImage)
			{
				process_save_cdg(*cdg, !config.showBorder);
				run.dumpImage = false;
			}
			if (run.nextChannel != run.channel) {
				cdg->setChannel(run.nextChannel);
				run.channel = run.nextChannel;
			}
		}
		run.nextTransport = T_NOCHANGE;

		tickdeltatime = SDL_GetTicks() - ticktime;
		//75 sectors per second
		//96 (24 * 4) subchannel data per sector
		//actually it's 98 bytes per subcode block
		//But the first two bytes are replaced by the sync pattern (so
		//P through W cannot be used then)
		if (run.transport == T_PLAY) {
			ticksnow = int(75.0 * 4.0 * ((double)tickdeltatime / 1000.0));
			cdg->execCount(run.cdgIn, ticksnow, dirty);
			run.cdgLoc += (24 * ticksnow);
			tickstotal += ticksnow;
			while (tickstotal < (((SDL_GetTicks() - loopbegintime - adjustmenttime) / 1000) * 300)) {
				cdg->execCount(run.cdgIn, 1, dirty);
				run.cdgLoc += 24;
				tickstotal++;
			}
		} else if (run.transport == T_PAUSE) {
			adjustmenttime += tickdeltatime;
		}

		ticktime = SDL_GetTicks();
		
		process_render(renderer, dirty, *cdg, &run, &config, ui);

		elapsedtime = SDL_GetTicks() - starttime;
		if (elapsedtime < 1) //stupid
			elapsedtime = 1;

		frame++;
		if(elapsedtime < 1000 / config.desiredFps) {
			//Sleep the remaining frame time
			SDL_Delay((1000 / config.desiredFps) - elapsedtime);
		} else {
			SDL_Delay(1);
		}

		if (run.transport == T_PLAY) {
			if (run.cdgIn.eof()) {
				run.nextTransport = T_STOP;
				if (config.quitOnEof)
					run.keeprunning = false;
			}
		}
	}

	run.cdgIn.close();
	if (config.audioEnabled && run.audioLoaded) {
		Mix_HaltMusic();
		if (run.audio != NULL) {
			Mix_FreeMusic(run.audio);
			run.audio = NULL;
		}
	}

	delete cdg;
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);

	cout << "\nBye." << endl;
	return 0;
}

void process_input(MainRunState *run, const MainConfig *config, CDGUI *ui) {
	//If there's an event to handle
	if(SDL_PollEvent(&(run->event))) {
		switch (run->event.type) {
			case SDL_QUIT:
				cout << "Recieved TERMINATE signal.\n";
				run->keeprunning = 0;
				break;
				
			case SDL_KEYDOWN:
				if (config->useKeys) {
					switch(run->event.key.keysym.sym) {
						case SDLK_q:
						case SDLK_ESCAPE: //quit
							run->keeprunning = 0; break;
						case SDLK_SPACE:
						case SDLK_c:
							run->nextTransport = (run->transport == T_PLAY) ? T_PAUSE : T_PLAY;
							break;
						case SDLK_r:
							run->nextTransport = T_STOP;
							/*in.close();
							in.open(config.file, ios::binary);
							cdg_init(&cdg, screen->format);*/
							break;
						case SDLK_F1:
							fl_message("%s", COPYRIGHT_MESSAGE);
							break;
						case SDLK_s:
							run->dumpImage = true;
							break;
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				//If the left mouse button was pressed
				switch (run->event.button.button) {
				case SDL_BUTTON_LEFT:
					if (config->useUI)
						ui->processMouseDown(run, config);
					break;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				//If the left mouse button was pressed
				switch (run->event.button.button) {
				case SDL_BUTTON_LEFT:
					if (config->useUI)
						ui->processMouseUp(run, config);
					break;
				}
				break;
		}
	}
}

void process_render(SDL_Renderer *renderer, int &dirty, const CDG &cdg, MainRunState *run, const MainConfig *config, CDGUI *ui) {
	if (run->cdgLoaded) {
		if (dirty) {
			cdg_render(cdg, renderer, !config->showBorder, config->scaling);
			dirty = 0;
		}
	} else {
		render_test_pattern(renderer, config->w, config->h);
	}

	if (config->useUI)
		ui->render(renderer, run, config);
	
	SDL_RenderPresent(renderer);
}

SDL_Window *graphics_init(MainConfig *config) {
	//Initialize all SDL subsystems
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		cerr << "FATAL> SDL failed to init\n";
		cerr << SDL_GetError();
		return NULL;
	}

	atexit(SDL_Quit);

	//Set up the screen
	int w = DEFAULT_WIDTH * config->scaling;
	int h = (DEFAULT_HEIGHT * config->scaling) + 
					((config->useUI) ? UI_HEIGHT : 0);
	if (config->woverride > 0)
		w = config->woverride;
	if (config->hoverride > 0)
		h = config->hoverride;
	
	SDL_Window *screen = SDL_CreateWindow(DEFAULT_CAPTION,
                          SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED,
                          w, h,
                          SDL_WINDOW_OPENGL | (config->fullscreen ? SDL_WINDOW_FULLSCREEN : 0));

	config->h = h;
	config->w = w;

	//If there was an error in setting up the screen
	if(screen == NULL) {
		cerr << "FATAL> screen did not set up properly\n";
		cerr << SDL_GetError();
		return NULL;
	}
	
	return screen;
}

int open_filechooser(MainRunState *run, bool music) {
	//from FLTK demo app
	Fl_Native_File_Chooser native;
	native.title("CD+G Deck - Pick a file");
	native.type(Fl_Native_File_Chooser::BROWSE_FILE);
	if (music)
		native.filter("All Music Files\t*\n");
	else
		native.filter("CD+G Graphics (.cdg)\t*.cdg\nAll Files\t*\n");
	//native.preset_file(G_filename->value());
	// Show native chooser
	switch (native.show()) {
		case -1: cerr << "ERROR: " << native.errmsg() << endl; exit(1); break; // ERROR
		case  1: /*exit(0);*/ break; // CANCEL
		default: // PICKED FILE
			if (native.filename()) {
				if (music) {
					run->audioFile = native.filename();
					run->audioLoaded = true;
				} else {
					run->cdgFile = native.filename();
					run->cdgLoaded = true;
					run->audioLoaded = false;

					stat(run->cdgFile.c_str(), &run->cdgStat);
					run->cdgLoc = 0;
				}
			} else {
				//null
			}
			return 1;
	}
	return 0;
}

void process_args(int argc, char* argv[], MainConfig *config) {
	for (int i = 1; i < argc; i++) {
		if (strcasecmp(argv[i], "-fps") == 0) {
			i++;
			if (i < argc) {
				config->desiredFps = atoi(argv[i]);
			}
		} else if (strcasecmp(argv[i], "-help") == 0) {
			cout << "usage: " << argv[0] << " [-help] [-quitonend] [-noaudio] [-noui] [-nokeys] [-autostart] [-seek (direct|enhanced)] [-scaling #] [-w #] [-h #] [-fullscreen|-windowed] [-audio file] [-] [file]" << endl;
			cout << COPYRIGHT_MESSAGE;
			exit(0);
		} else if (strcasecmp(argv[i], "-quitonend") == 0) {
			config->quitOnEof = true;
		} else if (strcasecmp(argv[i], "-noui") == 0) {
			config->useUI = false;
		} else if (strcasecmp(argv[i], "-nokeys") == 0) {
			config->useKeys = false;
		} else if (strcasecmp(argv[i], "-noaudio") == 0) {
			config->audioEnabled = false;
		} else if (strcasecmp(argv[i], "-autostart") == 0) {
			config->autostart = true;
		} else if (strcasecmp(argv[i], "-seek") == 0) {
			i++;
			if (i < argc) {
				if (strcasecmp(argv[i], "direct") == 0)
					config->seek = SeekMode::SEEK_DIRECT;
				else if (strcasecmp(argv[i], "enhanced") == 0)
					config->seek = SeekMode::SEEK_ENHANCED;
				else
					cerr << "Bad value for -seek ... use 'direct' or 'enhanced'" << endl;
			} else {
				cerr << "No value for -seek ... use 'direct' or 'enhanced'" << endl;
			}
		} else if (strcasecmp(argv[i], "-scaling") == 0) {
			i++;
			if (i < argc) {
				config->scaling = atoi(argv[i]);
			} else {
				cerr << "No value for -scaling" << endl;
			}
		} else if (strcasecmp(argv[i], "-fullscreen") == 0) {
			config->fullscreen = true;
		} else if (strcasecmp(argv[i], "-windowed") == 0) {
			config->fullscreen = false;
		} else if (strcasecmp(argv[i], "-w") == 0) {
			i++;
			if (i < argc) {
				config->woverride = atoi(argv[i]);
			} else {
				cerr << "No value for -w" << endl;
			}
		} else if (strcasecmp(argv[i], "-h") == 0) {
			i++;
			if (i < argc) {
				config->hoverride = atoi(argv[i]);
			} else {
				cerr << "No value for -h" << endl;
			}
		} else if (strcasecmp(argv[i], "-audio") == 0) {
			i++;
			if (i < argc) {
				config->autoAudioFile = argv[i];
				config->hasAutoAudioFile = true;
			} else {
				cerr << "No value for -audio" << endl;
			}
		} else if (strcasecmp(argv[i], "-showborder") == 0) {
			config->showBorder = true;
		} else if (strcasecmp(argv[i], "-gen") == 0) {
			exit(genmain());
		} else if (strcasecmp(argv[i], "-") == 0) {
			string s = "";
			for (int j = i + 1; j < argc; j++) {
				if (j != i + 1)
					s.append(" ");
				s.append(argv[j]);
			}
			config->autoFile = s;
			config->hasAutoFile = true;
			return;
		} else if (argv[i][0] == '-') {
			cerr << "Unknown option \"" << argv[i] << "\"" << endl;
		} else {
			config->autoFile = argv[i];
			config->hasAutoFile = true;
		}
	}
}

void process_save_cdg(const CDG & cdg, bool maskBorder)
{
	Fl_Native_File_Chooser native;
	native.title("CD+G Deck - Save screen capture");
	native.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	native.filter("PNG Image (.png)\t*.png\n");
	//native.preset_file(G_filename->value());
	// Show native chooser
	switch (native.show()) {
		case -1: cerr << "ERROR: " << native.errmsg() << endl; return; // ERROR
		case  1: return; // CANCEL
		default: // PICKED FILE
			if (native.filename()) {
				if (!save_cdg_screen(cdg, maskBorder, native.filename()))
				{
					fl_message("Failed to save image.");
				}
			}
	}
}
