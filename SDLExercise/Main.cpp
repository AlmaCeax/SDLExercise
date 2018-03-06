#include "../SDL/include/SDL.h"
#include "../SDL_Image/include/SDL_image.h"
#include "../SDL_Mixer/include/SDL_mixer.h"
#pragma comment(lib,"../SDL/libx86/SDL2.lib")
#pragma comment(lib,"../SDL/libx86/SDL2main.lib")
#pragma comment(lib,"../SDL_Image/libx86/SDL2_image.lib")
#pragma comment(lib,"../SDL_Mixer/libx86/SDL2_mixer.lib")

#define WIDTH 1280
#define HEIGHT 720

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define START 0
#define ERROR 1
#define RDY 2

#define BACKGROUND 0
#define SHIP 1
#define BULLET 2
#define BSD 3

#define SHOTS 10


struct ship {
	int x, y;
	bool directions[4];
	bool shooting;
	int speed;
	int shotTimer;
	int frame;
	SDL_Rect spriteClips[2];

	ship() {
		x = 32;
		y = HEIGHT / 2 -32;
		speed = 5;
		frame = 0;
		shotTimer = 0;
		shooting = false;
		for (bool &d : directions) {
			d = false;
		}
		spriteClips[0] = { 0, 0, 1024, 1024 };
		spriteClips[1] = { 0, 1052, 1024, 2048 };
	}
	bool canShoot() {
		if (shotTimer == 0) {
			shotTimer = 15;
			return true;
		}
		else {
			shotTimer--;
			return false;
		}
	}
};
struct bullet {
	int x, y;
	bool active, shot;
	int speed;

	bullet(int sp) {
		x = 0;
		y = 0;
		active = false;
		speed = sp;
	}
	bullet() {}
};

struct globals 
{
	SDL_Renderer* renderer = nullptr;
	Mix_Music* music = nullptr;
	Mix_Chunk* blaster = nullptr;
	Mix_Chunk* windows = nullptr;
	Mix_Chunk* error = nullptr;
	SDL_Window* window = nullptr;
	SDL_Texture* textures[4];
	int introState = START;

	ship* player = new ship();
	bullet playerBullets[SHOTS];
	bullet enemyBullets[SHOTS];
	SDL_Rect bckgdestRect = { 0,0,WIDTH,HEIGHT };
	int bsdTimer = 0;

	int playerShot = 0;
	int enemyShot = 10;

	bool bsdShoot() {
		if (bsdTimer == 0) {
			bsdTimer = 50;
			return true;
		}
		else {
			bsdTimer--;
			return false;
		}
	}
} g;




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
		Mix_PlayChannel(2, g.error, 0);
		g.introState = ERROR;
	}
	else {
		g.introState = RDY;
	}
}
void init() {
	SDL_Init(SDL_INIT_EVERYTHING);
	g.window = SDL_CreateWindow("SDLTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	g.renderer = SDL_CreateRenderer(g.window, -1, 0);

	IMG_Init(IMG_INIT_PNG);
	g.textures[BACKGROUND] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Assets/Images/background.png"));
	g.textures[SHIP] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Assets/Images/WinTack.png"));
	g.textures[BULLET] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Assets/Images/bullet.png"));
	g.textures[BSD] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Assets/Images/bsd.png"));

	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_U8, 2, 1024);
	g.music = Mix_LoadMUS("../Game/Assets/Sounds/song.ogg");
	Mix_VolumeMusic(50);
	//Mix_PlayMusic(g.music, -1);
	g.blaster = Mix_LoadWAV("../Game/Assets/Sounds/blaster.wav");
	g.windows = Mix_LoadWAV("../Game/Assets/Sounds/windows.wav");
	g.error = Mix_LoadWAV("../Game/Assets/Sounds/error.wav");

	Mix_PlayChannel(1, g.windows, 0);
	Mix_ChannelFinished(errorTime);


	for (int i = 0; i < SHOTS; i++) {
		g.playerBullets[i] = bullet(25);
		g.enemyBullets[i] = bullet(10);
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
				g.player->directions[UP] = true;
				break;
			case SDLK_DOWN:
				g.player->directions[DOWN] = true;
				break;
			case SDLK_LEFT:
				g.player->directions[LEFT] = true;
				break;
			case SDLK_RIGHT:
				g.player->directions[RIGHT] = true;
				break;
			case SDLK_SPACE:
				g.player->shooting = true;
				break;
			}
		}
		else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				g.player->directions[UP] = false;
				break;
			case SDLK_DOWN:
				g.player->directions[DOWN] = false;
				break;
			case SDLK_LEFT:
				g.player->directions[LEFT] = false;
				break;
			case SDLK_RIGHT:
				g.player->directions[RIGHT] = false;
				break;
			case SDLK_SPACE:
				g.player->shooting = false;
				break;
			}
		}
	}
}

void update() {

	if (g.introState == RDY) {
		if (g.player->directions[UP]) g.player->y -= g.player->speed;
		if (g.player->directions[DOWN]) g.player->y += g.player->speed;
		if (g.player->directions[LEFT]) g.player->x -= g.player->speed;
		if (g.player->directions[RIGHT]) g.player->x += g.player->speed;

		if (g.player->shooting) {
			if (g.player->canShoot()) {	
				Mix_PlayChannel(3, g.blaster, 0);
				if (g.playerShot == SHOTS) g.playerShot = 0;
				g.playerBullets[g.playerShot].active = true;
				g.playerBullets[g.playerShot].x = g.player->x + 32;
				g.playerBullets[g.playerShot].y = g.player->y + 16;
				g.playerShot++;
			}
		}

		if (g.bsdShoot()) {
			Mix_PlayChannel(4, g.blaster, 0);
			if (g.enemyShot == SHOTS) g.enemyShot = 0;
			g.enemyBullets[g.enemyShot].active = true;
			g.enemyBullets[g.enemyShot].x = WIDTH / 2;
			g.enemyBullets[g.enemyShot].y = g.player->y;
			g.enemyShot++;
		}

		for (int i = 0; i<SHOTS; i++)
		{
			if (g.playerBullets[i].active) {
				g.playerBullets[i].x += g.playerBullets[i].speed;
				if (g.playerBullets[i].x > WIDTH) g.playerBullets[i].active = false;
			}
			if (g.enemyBullets[i].active) {
				g.enemyBullets[i].x -= g.enemyBullets[i].speed;
				if (g.enemyBullets[i].x < 0) {
					g.enemyBullets[i].active = false;
				}
			}
		}
	}
}

void render() {
	SDL_Rect destRect;

	//background render
	destRect = {0, 0, WIDTH, HEIGHT};
	SDL_RenderCopy(g.renderer, g.textures[BACKGROUND], nullptr, &destRect);

	//bullet render
	for (int i = 0; i<10; i++)
	{
		if (g.playerBullets[i].active) {
			destRect = { g.playerBullets[i].x, g.playerBullets[i].y, 32, 32 };
			SDL_RenderCopy(g.renderer, g.textures[BULLET], nullptr, &destRect);
		}
		if (g.enemyBullets[i].active) {
			destRect = { g.enemyBullets[i].x, g.enemyBullets[i].y, 32, 32 };
			SDL_RenderCopy(g.renderer, g.textures[BULLET], nullptr, &destRect);
		}
	}

	if (g.introState != START)
	{
		destRect = { WIDTH / 2, HEIGHT / 4, WIDTH / 2, HEIGHT / 2 };
		SDL_RenderCopy(g.renderer, g.textures[BSD], nullptr, &destRect);
	}

	//Ship animation
	destRect = { g.player->x, g.player->y, 64, 64 };
	SDL_RenderCopy(g.renderer, g.textures[SHIP], &g.player->spriteClips[g.player->frame/6], &destRect);
	g.player->frame++;
	if (g.player->frame / 6 >= 2)
	{
		g.player->frame = 0;
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
