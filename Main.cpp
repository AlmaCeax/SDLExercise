#include "SDL\include\SDL.h"
#include "SDL_Image\include\SDL_image.h"
#include "SDL_Mixer\include\SDL_mixer.h"

#pragma comment( lib, "SDL/libx86/SDL2.lib" )
#pragma comment( lib, "SDL/libx86/SDL2main.lib" )
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )
#pragma comment( lib, "SDL_mixer/libx86/SDL2_mixer.lib" )


#define WIDTH 1280
#define HEIGHT 720

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define START 0
#define ERROR 1
#define PLAY 2
#define END 2

#define BACKGROUND 0
#define SHEET 1
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
	SDL_Rect collider;
	int lives = 3;

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
		collider = { x,y,64,64 };
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

struct globals 
{
	SDL_Renderer* renderer = nullptr;
	Mix_Music* music = nullptr;
	Mix_Chunk* blaster = nullptr;
	Mix_Chunk* windows = nullptr;
	Mix_Chunk* error = nullptr;
	SDL_Window* window = nullptr;
	SDL_Texture* textures[4];
	int gameState = START;

	ship* player = new ship();
	bullet playerBullets[SHOTS];
	bullet enemyBullets[SHOTS];
	SDL_Rect srcRectEB = { 1350, 1052, 1024, 1024 };
	SDL_Rect bsdCollider = { WIDTH / 2, HEIGHT / 4, WIDTH / 2, HEIGHT / 2 };
	int bsdTimer = 0;

	int playerShot = 0;
	int enemyShot = 10;

	bool bsdShoot() {
		if (bsdTimer == 0) {
			if (player->y > HEIGHT / 4 && player->y < HEIGHT / 4 + HEIGHT / 2 - 32)
			{
				bsdTimer = 50;
				return true;
			}
			return false;
		}
		bsdTimer--;
		return false;
	}
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
		Mix_PlayChannel(2, g.error, 0);
		g.gameState = ERROR;
	}
	else if (c==2) {
		g.gameState = PLAY;
	}
}
void restart() {
	g.gameState = START;
	g.player->x = 32;
	g.player->y = HEIGHT / 2 - 32;
	g.player->lives = 3;
	for (int i = 0; i < SHOTS; i++) {
		g.playerBullets[i].active = false;
		g.enemyBullets[i].active = false;
	}
	Mix_PlayChannel(1, g.windows, 0);
}

void init() {
	SDL_Init(SDL_INIT_EVERYTHING);
	g.window = SDL_CreateWindow("SDLTest", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	g.renderer = SDL_CreateRenderer(g.window, -1, 0);

	IMG_Init(IMG_INIT_PNG);
	g.textures[BACKGROUND] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/background.png"));
	g.textures[SHEET] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/WinTack.png"));
	g.textures[BULLET] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/bullet.png"));
	g.textures[BSD] = SDL_CreateTextureFromSurface(g.renderer, IMG_Load("Assets/Images/bsd.png"));

	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_U8, 2, 1024);
	g.music = Mix_LoadMUS("Assets/Sounds/song.ogg");
	Mix_VolumeMusic(50);
	//Mix_PlayMusic(g.music, -1);
	g.blaster = Mix_LoadWAV("Assets/Sounds/blaster.wav");
	g.windows = Mix_LoadWAV("Assets/Sounds/windows.wav");
	g.error = Mix_LoadWAV("Assets/Sounds/error.wav");

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

	if (g.gameState == PLAY) {
		if (g.player->directions[UP]) g.player->y -= g.player->speed;
		if (g.player->directions[DOWN]) g.player->y += g.player->speed;
		if (g.player->directions[LEFT]) g.player->x -= g.player->speed;
		if (g.player->directions[RIGHT]) g.player->x += g.player->speed;

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

		if (g.bsdShoot()) {
			Mix_PlayChannel(4, g.blaster, 0);
			if (g.enemyShot == SHOTS) g.enemyShot = 0;
			g.enemyBullets[g.enemyShot].active = true;
			g.enemyBullets[g.enemyShot].x = WIDTH / 2;
			g.enemyBullets[g.enemyShot].y = g.player->y;
			g.enemyBullets[g.enemyShot].collider.x = WIDTH / 2;
			g.enemyBullets[g.enemyShot].collider.y = g.player->y;
			g.enemyShot++;
		}

		for (int i = 0; i<SHOTS; i++)
		{
			if (g.playerBullets[i].active) {
				g.playerBullets[i].x += g.playerBullets[i].speed;
				g.playerBullets[i].collider.x = g.playerBullets[i].x;
				if (g.playerBullets[i].x > WIDTH) g.playerBullets[i].active = false;

				if (collision(g.playerBullets[i].collider, g.bsdCollider)) {
					g.playerBullets[i].active = false;
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
					g.player->lives--;
					if (g.player->lives == 0) restart();
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
			SDL_RenderCopy(g.renderer, g.textures[SHEET], &g.srcRectEB, &destRect);
		}
	}

	if (g.gameState != START)
	{
		destRect = { WIDTH / 2, HEIGHT / 4, WIDTH / 2, HEIGHT / 2 };
		SDL_RenderCopy(g.renderer, g.textures[BSD], nullptr, &destRect);
	}

	//Ship animation
	destRect = { g.player->x, g.player->y, 64, 64 };
	SDL_RenderCopy(g.renderer, g.textures[SHEET], &g.player->spriteClips[g.player->frame/6], &destRect);
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
