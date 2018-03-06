#include "../SDL/include/SDL.h"
#include "../SDL_Image/include/SDL_image.h"
#include "../SDL_Mixer/include/SDL_mixer.h"
#pragma comment(lib,"../SDL/libx86/SDL2.lib")
#pragma comment(lib,"../SDL/libx86/SDL2main.lib")
#pragma comment(lib,"../SDL_Image/libx86/SDL2_image.lib")
#pragma comment(lib,"../SDL_Mixer/libx86/SDL2_mixer.lib")

#define WIDTH 640
#define HEIGHT 480

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define BACKGROUND 0
#define SHIP 1
#define BULLET 2

#define SHOTS 10


struct ship {
	int x, y;
	bool directions[4];
	bool shooting;
	int speed;
	int shotTimer;
	int shotCD;

	ship() {
		x = WIDTH / 6;
		y = HEIGHT / 6;
		speed = 5;
		shotTimer = 0;
		shotCD = 10;
		shooting = false;
		for (bool &d : directions) {
			d = false;
		}
	}
	bool canShoot() {
		if (shotTimer > 0) return false;
		else return true;
	}
	void cooldown() {
		if (shotTimer > 0) shotTimer--;
	}
};
struct bullet {
	int x, y;
	bool active, shot;
	int speed;

	bullet() {
		x = 0;
		y = 0;
		active = false;
		speed = 25;
	}
};

struct globals 
{
	SDL_Renderer* renderer = nullptr;
	Mix_Music* music = nullptr;
	Mix_Chunk* blaster = nullptr;
	SDL_Window* window = nullptr;
	SDL_Texture* textures[3];


	ship* player = new ship();
	bullet bullets[SHOTS];
	SDL_Rect bckgdestRect = { 0,0,WIDTH,HEIGHT };
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

void init() {
	SDL_Init(SDL_INIT_EVERYTHING);
	g.window = SDL_CreateWindow("SDLTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	g.renderer = SDL_CreateRenderer(g.window, -1, 0);

	IMG_Init(IMG_INIT_PNG);
	g.textures[BACKGROUND] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Images/background.png"));
	g.textures[SHIP] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Images/ship.png"));
	g.textures[BULLET] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("../Game/Images/bullet.png"));

	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_U8, 2, 1024);
	g.music = Mix_LoadMUS("../Game/Sounds/song.ogg");
	Mix_VolumeMusic(50);
	Mix_PlayMusic(g.music, -1);
	g.blaster = Mix_LoadWAV("../Game/Sounds/blaster.wav");

	for (int i = 0; i < SHOTS; i++) {
		g.bullets[i] = bullet();
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
	if (g.player->directions[UP]) g.player->y -= g.player->speed;
	if (g.player->directions[DOWN]) g.player->y += g.player->speed;
	if (g.player->directions[LEFT]) g.player->x -= g.player->speed;
	if (g.player->directions[RIGHT]) g.player->x += g.player->speed;

	if (g.player->shooting) {
		if (g.player->canShoot()) {
			for (int i = 0; i < SHOTS; i++) {
				if (!g.bullets[i].active) {
					Mix_PlayChannel(-1, g.blaster, 0);
					g.bullets[i].active = true;
					g.bullets[i].x = g.player->x + 32;
					g.bullets[i].y = g. player->y + 16;
					g.player->shotTimer = g.player->shotCD;
					return;
				}
			}
		}
	}
	g.player->cooldown();

	for (int i = 0; i<SHOTS; i++)
	{
		if (g.bullets[i].active) {
			g.bullets[i].x += g.bullets[i].speed;
			if (g.bullets[i].x > WIDTH) g.bullets[i].active = false;
		}
	}
}

void render() {
	SDL_Rect destRect;
	destRect = {0, 0, WIDTH, HEIGHT};

	SDL_RenderCopy(g.renderer, g.textures[BACKGROUND], nullptr, &destRect);

	for (int i = 0; i<10; i++)
	{
		if (g.bullets[i].active) {
			destRect = { g.bullets[i].x, g.bullets[i].y, 32, 32 };
			SDL_RenderCopy(g.renderer, g.textures[BULLET], nullptr, &destRect);
		}
	}
	destRect = { g.player->x, g.player->y, 64, 64 };
	SDL_RenderCopy(g.renderer, g.textures[SHIP], nullptr, &destRect);

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
