#include "stdafx.h"
#include "cdg.h"
#include "cdgmain.h"
#include "cdgui.h"
#include "vera.h"

CDGUI::CDGUI() {
	TTF_Init();
	//if this fails we'll crash, but what do I care?
	//try http://www.fltk.org/newsgroups.php?gfltk.general+v:22083
	//now we're loading in from executable image
	_fontMem = SDL_RWFromMem(resource_Vera_TTF(), resource_Vera_TTF_len());
	font = TTF_OpenFontRW(_fontMem, 12, 12);
	if (!font)
	{
		cerr << "TTF_OpenFontRW: " << TTF_GetError() << endl;
		throw "Could not load font (see std err)";
	}

	resetButtonStates();
}

CDGUI::~CDGUI() {
	TTF_CloseFont(font);
	SDL_FreeRW(_fontMem);
	TTF_Quit();
	resetButtonStates();
	font = NULL;
}

void CDGUI::resetButtonStates() {
	btnPlayDown = btnLoadCDGDown = btnLoadAudioDown = btnSaveImageDown = btnPauseDown = btnStopDown = btnSeekbarDown = false;
}

Button CDGUI::mouseLocationToButton(const SDL_Event *event, const MainConfig *config) {
	int hoff = config->h - UI_HEIGHT;
	
	int x = event->button.x;
	if (event->button.y > hoff
		&& event->button.y < config->h - 1) {
		if (x < UI_WIDTH)
			return B_PLAY;
		else if (x < UI_WIDTH * 2)
			return B_PAUSE;
		else if (x < UI_WIDTH * 3)
			return B_STOP;
		else if (x < UI_WIDTH * 4)
			return B_LOADCDG;
		else if (x < UI_WIDTH * 5)
			return B_LOADAUDIO;
		else if (x < UI_WIDTH * 6)
			return B_SAVEIMAGE;
		else
			return B_SEEKBAR;
	}

	return B_NONE;
}

void CDGUI::processMouseDown(MainRunState *run, const MainConfig *config) {
	Button btn = mouseLocationToButton(&(run->event), config);

	switch (btn) {
	case B_PLAY:
		btnPlayDown = true;
		break;
	case B_PAUSE:
		btnPauseDown = true;
		break;
	case B_STOP:
		btnStopDown = true;
		break;
	case B_LOADCDG:
		btnLoadCDGDown = true;
		break;
	case B_LOADAUDIO:
		btnLoadAudioDown = true;
		break;
	case B_SAVEIMAGE:
		btnSaveImageDown = true;
		break;
	case B_SEEKBAR:
		btnSeekbarDown = true;
		break;
	}
}

void CDGUI::processMouseUp(MainRunState *run, const MainConfig *config) {
	Button btn = mouseLocationToButton(&(run->event), config);

	switch (btn) {
	case B_PLAY:
		if (btnPlayDown)
			run->nextTransport = T_PLAY;
		break;
	case B_PAUSE:
		if (btnPauseDown)
			run->nextTransport = T_PAUSE;
		break;
	case B_STOP:
		if (btnStopDown)
			run->nextTransport = T_STOP;
		break;
	case B_LOADCDG:
		if (btnLoadCDGDown)
			if (open_filechooser(run))
				run->nextTransport = T_STOP;
		break;
	case B_LOADAUDIO:
		if (btnLoadAudioDown)
			if (config->audioEnabled)
				if (open_filechooser(run, true))
					run->nextTransport = T_STOP;
		break;
	case B_SAVEIMAGE:
		if (btnSaveImageDown)
			run->dumpImage = true;
		break;
	case B_SEEKBAR:
		if (btnSeekbarDown) {
			double percent = double((UI_WIDTH * UI_NUM_BUTTONS) - run->event.button.x) / double((UI_WIDTH * UI_NUM_BUTTONS) - config->w);
			if (percent < 0.00000001) // random number
				percent = 0; //prevent -0% issues
			//cout << (percent * 100) << "%" << endl;
			run->nextLoc = percent;
		}
		break;
	}
	
	resetButtonStates();
}

void CDGUI::render(SDL_Renderer *renderer, const MainRunState *run, const MainConfig *config) {
	int hoff = config->h - UI_HEIGHT;
	char postext[80];
	
	SDL_Color text = {255, 255, 255};
	
	bool btnDown[] = {
		btnPlayDown,
		btnPauseDown,
		btnStopDown,
		btnLoadCDGDown,
		btnLoadAudioDown,
		btnSaveImageDown
	};
	const char* btnText[] = {
		"Play",
		"Pause",
		"Stop",
		"Open",
		"Audio",
		"SaveImg"
	};

	for (int i = 0; i < 6; i++) {
		SDL_Rect border;
		border.x = (UI_WIDTH * i);
		border.y = hoff;
		border.w = UI_WIDTH;
		border.h = UI_HEIGHT - 1;
		if (btnDown[i]) {
			SDL_SetRenderDrawColor(renderer, 83, 83, 83, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 122, 122, 122, 255);
		}
		SDL_RenderFillRect(renderer, &border);
		SDL_SetRenderDrawColor(renderer, 163, 163, 163, 255);
		SDL_RenderDrawRect(renderer, &border);
		
		render_text(renderer, (UI_WIDTH * i) + 4, hoff + 4, font, text, btnText[i]);
	}
	
	if (btnSeekbarDown) {
		SDL_SetRenderDrawColor(renderer, 83, 83, 83, 255);
	} else {
		SDL_SetRenderDrawColor(renderer, 163, 163, 163, 255);
	}
	SDL_Rect seekbarRect;
	seekbarRect.x = (UI_WIDTH * 6) + 1;
	seekbarRect.y = hoff + 1;
	seekbarRect.w = config->w - (UI_WIDTH * 6) - 1;
	seekbarRect.h = UI_HEIGHT - 1;
	SDL_RenderFillRect(renderer, &seekbarRect);
	if (run->cdgLoaded) {
		if (btnSeekbarDown) {
			SDL_SetRenderDrawColor(renderer, 44, 44, 44, 255);
		} else {
			SDL_SetRenderDrawColor(renderer, 83, 83, 83, 255);
		}
		SDL_Rect posRect;
		posRect.x = (UI_WIDTH * 6) + 1;
		posRect.y = hoff + 1;
		posRect.w = int((config->w - (UI_WIDTH * 6) - 1) * (run->cdgLoc / double(run->cdgStat.st_size)));
		posRect.h = UI_HEIGHT - 1;
		SDL_RenderFillRect(renderer, &posRect);
		
		int limsecs = CDG::sizeToSeconds(run->cdgStat.st_size);
		int cursecs = CDG::sizeToSeconds(run->cdgLoc);
		sprintf_s(postext, 78, "%i / %i (seconds)", cursecs, limsecs);
		render_text(renderer, (UI_WIDTH * 6) + 4, hoff + 4, font, text, postext);
	}
}