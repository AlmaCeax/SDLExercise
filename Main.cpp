#include "SDL\include\SDL.h"
#include "SDL_Image\include\SDL_image.h"
#include "SDL_Mixer\include\SDL_mixer.h"
#include <cstdlib>
#include <time.h> 

#pragma comment( lib, "SDL/libx86/SDL2.lib" )
#pragma comment( lib, "SDL/libx86/SDL2main.lib" )
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )
#pragma comment( lib, "SDL_mixer/libx86/SDL2_mixer.lib" )


#define WIDTH 1280
#define HEIGHT 720

#define READY 0
#define PLAYING 1
#define ERROR 2

#define BACKGROUND 0
#define SHEET 1
#define BSD 2
#define OBST 3
#define GO 4
#define VICTORY 5

#define SHOTS 10
#define OBSTACLES 30


struct ship {
	int x, y;
	bool up, down, left, right;
	bool shooting = false;
	int speed = 5;
	int shotTimer = 0;
	int frame = 0;
	SDL_Rect spriteClips[2];
	SDL_Rect collider;

	ship() {
		x = 32;
		y = HEIGHT / 2 -32;
		up = down = left = right = false;
		spriteClips[0] = { 0, 0, 1400, 1024 };
		spriteClips[1] = { 0, 1052, 1400, 2048 };
		collider = { x,y,32,32 };
	}
	bool canShoot() {
		if (shotTimer == 0 && shooting) {
			shotTimer = 15;
			return true;
		}
		if(shotTimer>0) shotTimer--;
		return false;
	}
};


struct bullet {
	int x, y;
	bool active, shot;
	int speed;
	SDL_Rect collider;

	bullet(int sp) {
		x = 0;
		y = 0;
		active = false;
		speed = sp;
		collider = { x,y,32,32 };
	}
	bullet() {}
};
struct obstacle {
	int x, y;
	bool active = true;
	SDL_Rect collider, srcRect;
};

struct bsd {
	SDL_Rect collider = { WIDTH / 2, 0, WIDTH / 2, HEIGHT };
	int timer = 0;
	int lives = 30;
	bool active = false;
	bool spawning = false;

	bool shoot() {
		if (timer == 0) {
			timer = 50;
			return true;
		}
		timer--;
		return false;
	}
};

struct globals
{
	SDL_Renderer* renderer = nullptr;
	Mix_Music* music = nullptr;
	Mix_Chunk* blaster = nullptr;
	Mix_Chunk* windows = nullptr;
	Mix_Chunk* error = nullptr;
	Mix_Chunk* kark = nullptr;
	SDL_Window* window = nullptr;
	SDL_Texture* textures[6];
	int gameState = READY;

	ship* player = new ship();
	bsd bsd;
	bullet playerBullets[SHOTS];
	bullet enemyBullets[SHOTS];
	obstacle obstacles[OBSTACLES];
	SDL_Rect srcBRect = { 1350, 1052, 1024, 1024 };
	SDL_Rect srcPRect = { 1350, 0, 1024, 1024 };

	int scroll = 0;
	int bckgWidth = 0;

	int playerShot = 0;
	int enemyShot = 10;
} g;


bool collision(SDL_Rect rect1, SDL_Rect rect2) {
	if ((rect1.x < rect2.x + rect2.w) &&
		(rect2.x < rect1.x + rect1.w) &&
		(rect1.y < rect2.y + rect2.h) &&
		(rect2.y < rect1.y + rect1.h)) {
		return true;
	}
	return false;
}

void close() {
	Mix_FreeMusic(g.music);
	Mix_CloseAudio();
	Mix_Quit();
	IMG_Quit();
	SDL_DestroyRenderer(g.renderer);
	SDL_DestroyWindow(g.window);
	SDL_Quit();
}

void errorTime(int c) {
	if (c == 1) {
		g.gameState = PLAYING;
		Mix_PlayMusic(g.music, -1);
	}
	else if (c==2) {
		g.gameState = PLAYING;
		g.bsd.active = true;
	}
}
void restart() {
	g.gameState = READY;
	g.player->x = 32;
	g.player->y = HEIGHT / 2 - 32;
	for (int i = 0; i < SHOTS; i++) {
		g.playerBullets[i].active = false;
		g.enemyBullets[i].active = false;
	}
	int lastX = WIDTH;
	for (int i = 0; i < OBSTACLES; i++) {
		int x = lastX + 250;
		int y = rand() % (HEIGHT - 128);
		g.obstacles[i].x = x;
		g.obstacles[i].y = y;
		int text = rand() % 4 + 1;
		switch (text) {
			case 1: g.obstacles[i].srcRect = { 0,0,57,80 }; break;
			case 2: g.obstacles[i].srcRect = { 70,0,65,80 }; break;
			case 3: g.obstacles[i].srcRect = { 135,0,57,80 }; break;
			case 4: g.obstacles[i].srcRect = { 190,0,75,80 }; break;
		}
		g.obstacles[i].collider = { x,y,57,80 };
		lastX = x;
	}
	g.scroll = 0;
	g.bsd.active = false;
	g.bsd.spawning = false;
	g.bsd.lives = 30;
	Mix_RewindMusic();
	Mix_PlayChannel(1, g.windows, 0);
}

void init() {
	SDL_Init(SDL_INIT_EVERYTHING);
	g.window = SDL_CreateWindow("SDLTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	g.renderer = SDL_CreateRenderer(g.window, -1, 0);

	IMG_Init(IMG_INIT_PNG);
	g.textures[BACKGROUND] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/Backgrobggund.png"));
	g.textures[SHEET] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/WinTack.png"));
	g.textures[BSD] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/bsd.png"));
	g.textures[OBST] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/Obstacles.png"));
	g.textures[GO] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/GameOver.png"));
	g.textures[VICTORY] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/Victory.png"));
	SDL_QueryTexture(g.textures[BACKGROUND], nullptr, nullptr, &g.bckgWidth, nullptr);

	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_U8, 2, 1024);
	g.music = Mix_LoadMUS("Assets/Sounds/music.ogg");
	Mix_VolumeMusic(35);
	g.blaster = Mix_LoadWAV("Assets/Sounds/blaster.wav");
	g.windows = Mix_LoadWAV("Assets/Sounds/windows.wav");
	g.error = Mix_LoadWAV("Assets/Sounds/error.wav");
	g.kark = Mix_LoadWAV("Assets/Sounds/kark.wav");
	Mix_VolumeChunk(g.blaster, 40);

	Mix_PlayChannel(1, g.windows, 0);
	Mix_ChannelFinished(errorTime);


	for (int i = 0; i < SHOTS; i++) {
		g.playerBullets[i] = bullet(25);
		g.enemyBullets[i] = bullet(10);
	}

	srand(time(NULL));

	int lastX = WIDTH;
	for (int i = 0; i < OBSTACLES; i++) {
		int x = lastX + 250;
		int y = rand() % (HEIGHT - 128);
		g.obstacles[i].x = x;
		g.obstacles[i].y = y;
		int text = rand() % 4 +1;
		switch (text) {
			case 1: g.obstacles[i].srcRect = { 0,0,57,80 }; break;
			case 2: g.obstacles[i].srcRect = { 70,0,65,80 }; break;
			case 3: g.obstacles[i].srcRect = { 135,0,57,80 }; break;
			case 4: g.obstacles[i].srcRect = { 190,0,75,80 }; break;
		}
		g.obstacles[i].collider = { x,y,57,80 };
		lastX = x;
	}
}

void handleEvents(bool &running)
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_QUIT) {
			running = false;
			return;
		}
		else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_UP:
				g.player->up = true;
				break;
			case SDLK_DOWN:
				g.player->down = true;
				break;
			case SDLK_LEFT:
				g.player->left = true;
				break;
			case SDLK_RIGHT:
				g.player->right = true;
				break;
			case SDLK_SPACE:
				g.player->shooting = true;
				break;
			case SDLK_RETURN:
				if (g.gameState == GO || g.gameState == VICTORY) restart();
			}
		}
		else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				g.player->up = false;
				break;
			case SDLK_DOWN:
				g.player->down = false;
				break;
			case SDLK_LEFT:
				g.player->left = false;
				break;
			case SDLK_RIGHT:
				g.player->right = false;
				break;
			case SDLK_SPACE:
				g.player->shooting = false;
				break;
			}
		}
	}
}

void update() {

	if (g.gameState == PLAYING) {
		if (g.player->up) g.player->y -= g.player->speed;
		if (g.player->down) g.player->y += g.player->speed;
		if (g.player->left) g.player->x -= g.player->speed;
		if (g.player->right) g.player->x += g.player->speed;

		if (g.player->y < 0)g.player->y = 0;
		if (g.player->y > HEIGHT-64) g.player->y = HEIGHT-64;
		if (g.player->x < 0)g.player->x = 0;
		if (g.player->x > WIDTH-64) g.player->x = WIDTH-64;

		g.player->collider.x = g.player->x;
		g.player->collider.y = g.player->y;

		if (g.player->canShoot()) {
			Mix_PlayChannel(3, g.blaster, 0);
			if (g.playerShot == SHOTS) g.playerShot = 0;
			g.playerBullets[g.playerShot].active = true;
			g.playerBullets[g.playerShot].x = g.player->x + 32;
			g.playerBullets[g.playerShot].y = g.player->y + 16;
			g.playerBullets[g.playerShot].collider.x = g.player->x + 32;
			g.playerBullets[g.playerShot].collider.y = g.player->y + 16;
			g.playerShot++;
		}

		if (g.bsd.active) {
			if (g.bsd.shoot()) {
				Mix_PlayChannel(4, g.kark, 0);
				if (g.enemyShot == SHOTS) g.enemyShot = 0;
				g.enemyBullets[g.enemyShot].active = true;
				g.enemyBullets[g.enemyShot].x = WIDTH / 2;
				g.enemyBullets[g.enemyShot].y = g.player->y;
				g.enemyBullets[g.enemyShot].collider.x = WIDTH / 2;
				g.enemyBullets[g.enemyShot].collider.y = g.player->y;
				g.enemyShot++;
			}
		}

		if (!g.bsd.active) {
			for (int i = 0; i < OBSTACLES; i++) {
				if (g.obstacles[i].x > -128) {
					g.obstacles[i].x -= 5;
					g.obstacles[i].collider.x -= 5;

					if (g.obstacles[i].x >= (WIDTH + 128)) g.obstacles[i].active = true;
					if (g.obstacles[i].active) {
						if (g.obstacles[i].x < -128) g.obstacles[i].active = false;
						if (collision(g.obstacles[i].collider, g.player->collider)) {
							g.obstacles[i].active = false;
							Mix_FadeOutMusic(1000);
							g.gameState = GO; 
						}
					}
				}
			}
		}
		else {
			if (collision(g.bsd.collider, g.player->collider)) {
				Mix_FadeOutMusic(1000);
				g.gameState = GO;
			}
		}
		for (int i = 0; i<SHOTS; i++)
		{
			if (g.playerBullets[i].active) {
				g.playerBullets[i].x += g.playerBullets[i].speed;
				g.playerBullets[i].collider.x = g.playerBullets[i].x;
				if (g.playerBullets[i].x > WIDTH) g.playerBullets[i].active = false;

				if (g.bsd.active) {
					if(collision(g.playerBullets[i].collider, g.bsd.collider)) {
						g.playerBullets[i].active = false;
						g.bsd.lives--;
						if (g.bsd.lives == 0) {
							Mix_FadeOutMusic(1000);
							g.gameState = VICTORY;
						}
					}
				}
				else
				{
					for (int ii = 0; ii < OBSTACLES; ii++) {
						if (g.obstacles[ii].active) {
							if (collision(g.obstacles[ii].collider, g.playerBullets[i].collider)) {
								g.obstacles[ii].active = false;
								g.obstacles[ii].x = -128;
								g.playerBullets[i].active = false;
							}
						}
					}
				}
			}
			if (g.enemyBullets[i].active) {
				g.enemyBullets[i].x -= g.enemyBullets[i].speed;
				g.enemyBullets[i].collider.x = g.enemyBullets[i].x;
				if (g.enemyBullets[i].x < 0) {
					g.enemyBullets[i].active = false;
				}
				if (collision(g.enemyBullets[i].collider, g.player->collider)) {
					g.enemyBullets[i].active = false;
					Mix_FadeOutMusic(1000);
					g.gameState = GO; 
				}
			}
		}

		if (!g.bsd.spawning) {
			bool ok = true;
			for (int i = (OBSTACLES - 5); i < OBSTACLES; i++) {
				if (g.obstacles[i].x > -128) ok = false;
			}
			if (ok) {
				Mix_FadeOutMusic(1000);
				Mix_PlayChannel(2, g.error, 0);
				g.bsd.spawning = true;
			}
		}	
	}
}

void render() {
	SDL_Rect destRect;

	if(g.gameState != VICTORY && g.gameState != GO)
	{
		if (!g.bsd.spawning && g.gameState == PLAYING) {
			g.scroll -= 5;
			if (g.scroll < -g.bckgWidth) g.scroll = 0;

			destRect = { g.scroll, 0, g.bckgWidth, HEIGHT };

			SDL_RenderCopy(g.renderer, g.textures[BACKGROUND], nullptr, &destRect);
			destRect.x += g.bckgWidth;
			SDL_RenderCopy(g.renderer, g.textures[BACKGROUND], nullptr, &destRect);
		}
		else {
			destRect = { g.scroll, 0, g.bckgWidth, HEIGHT };
			SDL_RenderCopy(g.renderer, g.textures[BACKGROUND], nullptr, &destRect);
		}
		//bullet render
		for (int i = 0; i<10; i++)
		{
			if (g.playerBullets[i].active) {
				destRect = { g.playerBullets[i].x, g.playerBullets[i].y, 32, 32 };
				SDL_RenderCopy(g.renderer, g.textures[SHEET], &g.srcPRect, &destRect);
			}
			if (g.enemyBullets[i].active) {
				destRect = { g.enemyBullets[i].x, g.enemyBullets[i].y, 32, 32 };
				SDL_RenderCopy(g.renderer, g.textures[SHEET], &g.srcBRect, &destRect);
			}
		}

		for (int i = 0; i < OBSTACLES; i++) {
			if (g.obstacles[i].active) {
				destRect = { g.obstacles[i].x, g.obstacles[i].y, 128, 128 };
				SDL_RenderCopy(g.renderer, g.textures[OBST], &g.obstacles[i].srcRect, &destRect);
			}
		}

		if (g.bsd.active)
		{
			destRect = { WIDTH / 2, 0, WIDTH / 2, HEIGHT };
			SDL_RenderCopy(g.renderer, g.textures[BSD], nullptr, &destRect);
		}

		//Ship animation
		destRect = { g.player->x, g.player->y, 64, 64 };
		SDL_RenderCopy(g.renderer, g.textures[SHEET], &g.player->spriteClips[g.player->frame / 6], &destRect);
		g.player->frame++;
		if (g.player->frame / 6 >= 2)
		{
			g.player->frame = 0;
		}
	}
		

	if (g.gameState == GO) {
		destRect = { 0, 0, WIDTH, HEIGHT };
		SDL_RenderCopy(g.renderer, g.textures[GO], nullptr, &destRect);
	}
	else if(g.gameState == VICTORY){
		destRect = { 0, 0, WIDTH, HEIGHT };
		SDL_RenderCopy(g.renderer, g.textures[VICTORY], nullptr, &destRect);
	}
	SDL_RenderPresent(g.renderer);
}



int main(int argc, char* argv[])
{
	bool running = false;
	const int FPS = 60;
	const int frameDelay = 1000 / FPS;

	init();
	running = true;

	Uint32 frameStart;
	int frameTime;

	while (running) {

		frameStart = SDL_GetTicks();

		handleEvents(running);
		update();
		render();

		frameTime = SDL_GetTicks() - frameStart;

		if (frameDelay > frameTime) {
			SDL_Delay(frameDelay - frameTime);
		}
	}

	close();

	return 0;
}
