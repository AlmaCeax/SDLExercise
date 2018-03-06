#include "../SDL/include/SDL.h"
#pragma comment(lib,"../SDL/lib/x86/SDL2.lib")
#pragma comment(lib,"../SDL/lib/x86/SDL2main.lib")

#include "../SDL_Image/include/SDL_image.h"
#pragma comment(lib,"../SDL_Image/lib/x86/SDL2_image.lib")

#include "../SDL_Mixer/include/SDL_mixer.h"
#pragma comment(lib,"../SDL_Mixer/lib/x86/SDL2_mixer.lib")


#define WIDTH 640
#define HEIGHT 480
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define BACKGROUND 0
#define SHIP 1
#define BULLET 2

SDL_Renderer* renderer = nullptr;
Mix_Music* music = nullptr;
Mix_Chunk* blaster = nullptr;

struct ship {
	int x, y;
	bool directions[4];
	bool shooting;
	int speed;
	int shotTimer;
	int shotCD;
	SDL_Rect* destRect;
	SDL_Rect* srcRect;

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
		destRect = new SDL_Rect();
		destRect->x = x;
		destRect->y = y;
		destRect->w = WIDTH / 4;
		destRect->h = HEIGHT / 4;

		srcRect = new SDL_Rect();
		srcRect->x = 0;
		srcRect->y = 0;
		srcRect->w = 960;
		srcRect->h = 506;
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
	SDL_Rect destRect;
	SDL_Rect srcRect;

	bullet() {
		x = 0;
		y = 0;
		active = false;
		speed = 25;
		destRect = { x , y, 42, 32 };
		srcRect = { 0, 0,220,1254 };
	}
};

void close(SDL_Window* window) {
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	IMG_Quit();
	Mix_FreeMusic(music);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
}

void init(SDL_Window* window, bullet bullets[10], SDL_Texture* textures[3]) {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	Mix_Init(MIX_INIT_OGG);
	Mix_AllocateChannels(16);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_U8, 2, 1024);
	window = SDL_CreateWindow("SDLTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_Surface* surf;
	surf = IMG_Load("../Game/Images/background.png");
	if (!surf) close(window);
	textures[BACKGROUND] = SDL_CreateTextureFromSurface(renderer, surf);
	surf = IMG_Load("../Game/Images/ship.png");
	if (!surf) close(window);
	textures[SHIP] = SDL_CreateTextureFromSurface(renderer, surf);
	surf = IMG_Load("../Game/Images/bullet.png");
	if (!surf) close(window);
	textures[BULLET] = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);

	music = Mix_LoadMUS("../Game/Sounds/song.ogg");
	Mix_VolumeMusic(50);
	Mix_PlayMusic(music, -1);

	blaster = Mix_LoadWAV("../Game/Sounds/blaster.wav");

	for (int i = 0; i < 10; i++) {
		bullets[i] = bullet();
	}
}

void handleEvents(bool &running, ship* player)
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
				player->directions[UP] = true;
				break;
			case SDLK_DOWN:
				player->directions[DOWN] = true;
				break;
			case SDLK_LEFT:
				player->directions[LEFT] = true;
				break;
			case SDLK_RIGHT:
				player->directions[RIGHT] = true;
				break;
			case SDLK_SPACE:
				player->shooting = true;
				break;
			}
		}
		else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				player->directions[UP] = false;
				break;
			case SDLK_DOWN:
				player->directions[DOWN] = false;
				break;
			case SDLK_LEFT:
				player->directions[LEFT] = false;
				break;
			case SDLK_RIGHT:
				player->directions[RIGHT] = false;
				break;
			case SDLK_SPACE:
				player->shooting = false;
				break;
			}
		}
	}
}

void update(ship* player, bullet bullets[10]) {
	if (player->directions[UP]) player->y -= player->speed;
	if (player->directions[DOWN]) player->y += player->speed;
	if (player->directions[LEFT]) player->x -= player->speed;
	if (player->directions[RIGHT]) player->x += player->speed;

	if (player->shooting) {
		if (player->canShoot()) {
			for (int i = 0; i < 10; i++) {
				if (!bullets[i].active) {
					Mix_PlayChannel(-1, blaster, 0);
					bullets[i].active = true;
					bullets[i].x = player->x + player->destRect->w;
					bullets[i].y = player->y + (player->destRect->h / 2 - bullets[i].destRect.h);
					bullets[i].destRect.x = bullets[i].x;
					bullets[i].destRect.y = bullets[i].y;
					player->shotTimer = player->shotCD;
					return;
				}
			}
		}
	}
	player->cooldown();

	player->destRect->x = player->x;
	player->destRect->y = player->y;

	for (int i = 0; i<10; i++)
	{
		if (bullets[i].active) {
			bullets[i].x += bullets[i].speed;
			bullets[i].destRect.x = bullets[i].x;
			if (bullets[i].x > WIDTH) bullets[i].active = false;
		}
	}
}

void render(ship* player, bullet bullets[10], SDL_Texture* textures[3], SDL_Rect *bckgsrcRect, SDL_Rect *bckgdestRect) {

	SDL_RenderCopy(renderer, textures[BACKGROUND], bckgsrcRect, bckgdestRect);

	for (int i = 0; i<10; i++)
	{
		if (bullets[i].active) {
			SDL_RenderCopy(renderer, textures[BULLET], &bullets[i].srcRect, &bullets[i].destRect);
		}
	}

	SDL_RenderCopy(renderer, textures[SHIP], player->srcRect, player->destRect);

	SDL_RenderPresent(renderer);
}



int main(int argc, char* argv[])
{
	SDL_Window* window = nullptr;
	SDL_Texture* textures[3];

	SDL_Rect bckgsrcRect = { 0,0,1200,717 };
	SDL_Rect bckgdestRect = { 0,0,WIDTH,HEIGHT };



	ship* player = new ship();
	bullet bullets[10];

	bool running = false;
	const int FPS = 60;
	const int frameDelay = 1000 / FPS;

	init(window, bullets, textures);
	running = true;

	Uint32 frameStart;
	int frameTime;

	while (running) {

		frameStart = SDL_GetTicks();

		handleEvents(running, player);
		update(player, bullets);
		render(player, bullets, textures, &bckgsrcRect, &bckgdestRect);

		frameTime = SDL_GetTicks() - frameStart;

		if (frameDelay > frameTime) {
			SDL_Delay(frameDelay - frameTime);
		}
	}

	close(window);

	return 0;
}
