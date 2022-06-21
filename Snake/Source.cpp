#include <chrono>
#include <thread>
#include <cstdlib>
#include <vector>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

using namespace std;

class Snake : public olc::PixelGameEngine
{
public:
	Snake()
	{
		sAppName = "Snake";
	}

private:
	bool menue;
	bool game;
	bool gameOver;

	int sWidth, sHeight;
	int blockWidth;
	int offsetX, offsetY;
	int appleWidth;
	float snakeSpeed;

	float t;

	int dir;
	int length;
	vector<olc::vi2d> snake;
	vector<olc::vi2d> apple;
	vector<olc::Pixel> snakeColors;

	bool almostDead;

	vector<int> moves;
	int mode;

	bool bToggle = false;
	static bool bSynthPlaying;
	static float fSynthFrequency;
	static float fFilterVolume;

	static float fPreviousSamples[128];
	static int nSamplePos;

	int moveSound;
	int pointSound;

	int clickSound;
	int menueSelect;
	int gameOverSound;
	int gameMusic;

	int currentSong;
	vector<int> songs;


public:
	static float MyCustomSynthFunction(int nChannel, float fGlobalTime, float fTimeStep)
	{
		// Just generate a sine wave of the appropriate frequency
		if (bSynthPlaying)
			return sin(fSynthFrequency * 2.0f * 3.14159f * fGlobalTime);
		else
			return 0.0f;
	}

	static float MyCustomFilterFunction(int nChannel, float fGlobalTime, float fSample)
	{
		// Fundamentally just control volume
		float fOutput = fSample * fFilterVolume;

		// But also add sample to list of previous samples for visualisation
		fPreviousSamples[nSamplePos] = fOutput;
		nSamplePos++;
		nSamplePos %= 128;

		return fOutput;
	}

	bool OnUserCreate() override
	{
		menue = true;
		game = false;
		gameOver = false;

		sWidth = sHeight = 18;
		blockWidth = 55;
		appleWidth = 35;

		offsetX = ScreenWidth() / 2 - blockWidth * (sWidth / 2);
		offsetY = ScreenHeight() / 2 - blockWidth * (sHeight / 2);

		length = 2;
		dir = 3;
		snake = { olc::vi2d(1, 8), olc::vi2d(2, 8) };
		apple = { olc::vi2d(13, 8) };
		snakeSpeed = (float)sWidth / 3.0f;

		almostDead = false;
		mode = 0;

		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);
		moveSound = olc::SOUND::LoadAudioSample("sfx/moveSound.wav");
		pointSound = olc::SOUND::LoadAudioSample("sfx/appleSound.wav");
		clickSound = olc::SOUND::LoadAudioSample("sfx/clickSound.wav");
		menueSelect = olc::SOUND::LoadAudioSample("sfx/menueSelect.wav");
		gameOverSound = olc::SOUND::LoadAudioSample("sfx/gameOver.wav");

		songs.push_back(olc::SOUND::LoadAudioSample("music/StreetLove.wav"));
		songs.push_back(olc::SOUND::LoadAudioSample("music/InfiniteDoors.wav"));
		songs.push_back(olc::SOUND::LoadAudioSample("music/Zephyr.wav"));
		songs.push_back(olc::SOUND::LoadAudioSample("music/Potato.wav"));

		olc::SOUND::SetUserSynthFunction(MyCustomSynthFunction);

		olc::SOUND::SetUserFilterFunction(MyCustomFilterFunction);

		currentSong = 0;
		olc::SOUND::PlaySample(songs[currentSong]);

		srand((unsigned)time(NULL));

		return true;
	}

	float hue2rgb(float p, float q, float t) {

		if (t < 0)
			t += 1;
		if (t > 1)
			t -= 1;
		if (t < 1. / 6)
			return p + (q - p) * 6 * t;
		if (t < 1. / 2)
			return q;
		if (t < 2. / 3)
			return p + (q - p) * (2. / 3 - t) * 6;

		return p;
	}

	olc::Pixel hsl2rgb(float h, float s, float l) {

		olc::Pixel result;

		if (0 == s) {
			result.r = result.g = result.b = l; // achromatic
		}
		else {
			float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
			float p = 2 * l - q;
			result.r = hue2rgb(p, q, h + 1. / 3) * 255;
			result.g = hue2rgb(p, q, h) * 255;
			result.b = hue2rgb(p, q, h - 1. / 3) * 255;
		}

		return result;
	}

	void drawBlock(olc::vi2d pos, olc::Pixel color, int width, int height)
	{
		olc::vi2d rectPos = {pos.x * blockWidth + offsetX + (blockWidth - width) / 2, pos.y* blockWidth + offsetY + (blockWidth - height) / 2 };
		FillRect(rectPos, olc::vi2d(width, height), color);
	}

	olc::vi2d spawApple()
	{
		int aX, aY;
		aX = rand() % sWidth;
		aY = rand() % sHeight;

		for (int i = 0; i < snake.size(); i++)
		{
			if (snake[i].x == aX && snake[i].y == aY)
			{
				olc::vi2d coords = spawApple();
				aX = coords.x;
				aY = coords.y;
			}
		}
		return olc::vi2d(aX, aY);
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		this_thread::sleep_for(chrono::milliseconds((long)((1.0f / 60.0f - fElapsedTime) * 1000.0f)));
		if (olc::SOUND::listActiveSamples.size() == 0)
		{
			currentSong++;
			if (currentSong < songs.size())
			{
				olc::SOUND::PlaySample(songs[currentSong]);
			}
		}

		auto PointInRect = [&](int x, int y, int rx, int ry, int rw, int rh)
		{
			return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
		};

		if (game)
		{
			// UP: 0, DOWN: 1, LEFT: 2, RIGHT: 3
			if (GetKey(olc::Key::UP).bPressed)
			{
				moves.push_back(0);
			}
			else if (GetKey(olc::Key::DOWN).bPressed)
			{
				moves.push_back(1);
			}
			else if (GetKey(olc::Key::LEFT).bPressed)
			{
				moves.push_back(2);
			}
			else if (GetKey(olc::Key::RIGHT).bPressed)
			{
				moves.push_back(3);
			}

			if (GetKey(olc::Key::W).bPressed)
			{
				moves.push_back(0);
			}
			else if (GetKey(olc::Key::S).bPressed)
			{
				moves.push_back(1);
			}
			else if (GetKey(olc::Key::A).bPressed)
			{
				moves.push_back(2);
			}
			else if (GetKey(olc::Key::D).bPressed)
			{
				moves.push_back(3);
			}

			if (GetKey(olc::Key::M).bPressed)
				mode = mode == 0 ? 1 : 0;

			t += fElapsedTime;
			if (t > 0.1)
			{
				t = fmod(t, 0.1);
				if (!moves.empty())
				{
					if (dir != moves.front())
						if (!(dir == 0 && moves.front() == 1) && !(dir == 1 && moves.front() == 0))
							if (!(dir == 2 && moves.front() == 3) && !(dir == 3 && moves.front() == 2))
							{
								dir = moves.front();
								olc::SOUND::PlaySample(moveSound);
							}
					moves.erase(moves.begin());
				}
				if (dir == 0)
					snake.push_back(olc::vi2d(snake.back().x, snake.back().y - 1));
				else if (dir == 1)
					snake.push_back(olc::vi2d(snake.back().x, snake.back().y + 1));
				else if (dir == 2)
					snake.push_back(olc::vi2d(snake.back().x - 1, snake.back().y));
				else if (dir == 3)
					snake.push_back(olc::vi2d(snake.back().x + 1, snake.back().y));

				if (moves.size() > 3)
				{
					vector<int> y(moves.end() - 3, moves.end());
					moves = y;
				}

				int x = snake.back().x, y = snake.back().y;
				if (x == sWidth || y == sHeight || x == -1 || y == -1)
				{
					if (almostDead)
					{
						game = false;
						menue = false;
						gameOver = true;
						snake.pop_back();
						olc::SOUND::PlaySample(gameOverSound);
					}
					else
					{
						almostDead = true;
						snake.pop_back();
					}
				}
				else
					almostDead = false;

				for (int i = 0; i < apple.size(); i++)
				{
					if (apple[i].x == x && apple[i].y == y)
					{
						length++;
						apple.erase(apple.begin() + i);

						olc::SOUND::PlaySample(pointSound);

						olc::vi2d appleCoords = spawApple();
						apple.push_back(appleCoords);
					}
				}

				for (int i = 0; i < snake.size() - 1; i++)
				{
					if (x == snake[i].x && y == snake[i].y)
					{
						game = false;
						menue = false;
						gameOver = true;
						snake.pop_back();
						olc::SOUND::PlaySample(gameOverSound);
					}
				}

				if (snake.size() > length)
					snake.erase(snake.begin());

				// Draw
				Clear(olc::BLACK);

				for (int y = 0; y < sHeight; y++)
					for (int x = 0; x < sWidth; x++)
					{
						olc::Pixel color;
						float light[3] = { 0.0f, 0.0f, 0.0f };

						if (mode == 0)
						{
							color = ((x + y) % 2 == 0) ? olc::Pixel(17, 17, 17) : olc::Pixel(34, 34, 34);
							for (int i = 0; i < snake.size(); i++)
							{
								float distance = roundf(sqrtf((x - snake[i].x) * (x - snake[i].x) + (y - snake[i].y) * (y - snake[i].y)));
								if (distance)
								{
									olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
									light[0] += (float)(snakeColor.r / 255.0f) / (distance * distance);
									light[1] += (float)(snakeColor.g / 255.0f) / (distance * distance);
									light[2] += (float)(snakeColor.b / 255.0f) / (distance * distance);
								}
							}

							float lightMagnitude = sqrt(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
							if (lightMagnitude > 1.0f)
							{
								color.r *= 1.0f + light[0] / lightMagnitude;
								color.g *= 1.0f + light[1] / lightMagnitude;
								color.b *= 1.0f + light[2] / lightMagnitude;
							}
							else
							{
								color.r *= 1.0f + light[0];
								color.g *= 1.0f + light[1];
								color.b *= 1.0f + light[2];
							}
						}
						else
						{
							color = olc::Pixel(17, 17, 17);
							for (int i = 0; i < snake.size(); i++)
							{
								float distance = roundf(sqrtf((x - snake[i].x) * (x - snake[i].x) + (y - snake[i].y) * (y - snake[i].y)));
								if (distance)
								{
									olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
									light[0] += (float)(snakeColor.r / 255.0f) / (distance * distance);
									light[1] += (float)(snakeColor.g / 255.0f) / (distance * distance);
									light[2] += (float)(snakeColor.b / 255.0f) / (distance * distance);
								}
							}

							float lightMagnitude = sqrt(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
							if (lightMagnitude > 1.0f)
							{
								color.r *= 1.0f + light[0] / lightMagnitude;
								color.g *= 1.0f + light[1] / lightMagnitude;
								color.b *= 1.0f + light[2] / lightMagnitude;
							}
							else
							{
								color.r *= 1.0f + light[0];
								color.g *= 1.0f + light[1];
								color.b *= 1.0f + light[2];
							}
						}

						drawBlock(olc::vi2d(x, y), color, blockWidth, blockWidth);
					}

				olc::Pixel color = { 232, 136, 58 };
				for (int i = 0; i < apple.size(); i++)
				{
					drawBlock(apple[i], color, appleWidth, appleWidth);
				}


				for (int i = 0; i < snake.size(); i++)
				{
					olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
					drawBlock(snake[i], snakeColor, blockWidth, blockWidth);
				}

				DrawString(olc::vi2d(ScreenWidth() / 2 - ((8 + log10(snake.size())) * 8 * 4) / 2, 0), "Score: " + std::to_string(snake.size()), olc::WHITE, 4);
			}
		}

		if (gameOver)
		{
			Clear(olc::BLACK);

			t += fElapsedTime / 5.0f;

			if (t > 1.0f)
			{
				t = fmod(t, 1.0f);
			}

			for (int y = 0; y < sHeight; y++)
				for (int x = 0; x < sWidth; x++)
				{
					olc::Pixel color;
					float light[3] = { 0.0f, 0.0f, 0.0f };

					if (mode == 0)
					{
						color = ((x + y) % 2 == 0) ? olc::Pixel(17, 17, 17) : olc::Pixel(34, 34, 34);
						for (int i = 0; i < snake.size(); i++)
						{
							float distance = roundf(sqrtf((x - snake[i].x) * (x - snake[i].x) + (y - snake[i].y) * (y - snake[i].y)));
							if (distance)
							{
								olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
								light[0] += (float)(snakeColor.r / 255.0f) / (distance * distance);
								light[1] += (float)(snakeColor.g / 255.0f) / (distance * distance);
								light[2] += (float)(snakeColor.b / 255.0f) / (distance * distance);
							}
						}

						float lightMagnitude = sqrt(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
						if (lightMagnitude > 1.0f)
						{
							color.r *= 1.0f + light[0] / lightMagnitude;
							color.g *= 1.0f + light[1] / lightMagnitude;
							color.b *= 1.0f + light[2] / lightMagnitude;
						}
						else
						{
							color.r *= 1.0f + light[0];
							color.g *= 1.0f + light[1];
							color.b *= 1.0f + light[2];
						}
					}
					else
					{
						color = olc::Pixel(17, 17, 17);
						for (int i = 0; i < snake.size(); i++)
						{
							float distance = roundf(sqrtf((x - snake[i].x) * (x - snake[i].x) + (y - snake[i].y) * (y - snake[i].y)));
							if (distance)
							{
								olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
								light[0] += (float)(snakeColor.r / 255.0f) / (distance * distance);
								light[1] += (float)(snakeColor.g / 255.0f) / (distance * distance);
								light[2] += (float)(snakeColor.b / 255.0f) / (distance * distance);
							}
						}

						float lightMagnitude = sqrt(light[0] * light[0] + light[1] * light[1] + light[2] * light[2]);
						if (lightMagnitude > 1.0f)
						{
							color.r *= 1.0f + light[0] / lightMagnitude;
							color.g *= 1.0f + light[1] / lightMagnitude;
							color.b *= 1.0f + light[2] / lightMagnitude;
						}
						else
						{
							color.r *= 1.0f + light[0];
							color.g *= 1.0f + light[1];
							color.b *= 1.0f + light[2];
						}
					}

					drawBlock(olc::vi2d(x, y), color, blockWidth, blockWidth);
				}

			olc::Pixel color = { 232, 136, 58 };
			for (int i = 0; i < apple.size(); i++)
			{
				drawBlock(apple[i], color, appleWidth, appleWidth);
			}


			for (int i = 0; i < snake.size(); i++)
			{
				olc::Pixel snakeColor = hsl2rgb(fmod((snake.size() - 1 - i) * 1.0f / (sWidth * sHeight) * 8.0f, 1.0f), 1.0f, 0.5f);
				drawBlock(snake[i], snakeColor, blockWidth, blockWidth);
			}

			vector<string> title = { "G", "a", "m", "e", " ", "O", "v", "e", "r" };
			for (int i = 0; i < title.size(); i++)
			{
				DrawString(olc::vi2d(ScreenWidth() / 2 - (title.size() * 8 * 10) / 2 + i * 8 * 10, ScreenHeight() / 2 - 120), title[i], hsl2rgb(fmod(t + i * 1.0f / (float)title.size(), 1.0f), 1.0f, 0.5f), 10);
			}

			DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 15), "Play", olc::WHITE, 6);

			DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 45 + 8 * 6), "Menu", olc::WHITE, 6);

			DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 75 + 2 * 8 * 6), "Quit", olc::WHITE, 6);

			if (GetMouse(0).bPressed)
			{
				int x = GetMouseX(), y = GetMouseY();
				if (PointInRect(x, y, ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 15, (4 * 8 * 6), 8 * 6))
				{
					olc::SOUND::PlaySample(clickSound);

					length = 2;
					dir = 3;
					snake = { olc::vi2d(1, 8), olc::vi2d(2, 8) };
					apple = { olc::vi2d(13, 8) };
					snakeSpeed = (float)sWidth / 3.0f;

					almostDead = false;
					mode = 0;

					game = true;
					gameOver = false;
					menue = false;
				}

				if (PointInRect(x, y, ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 45 + 8 * 6, (4 * 8 * 6), 8 * 6))
				{
					olc::SOUND::PlaySample(clickSound);

					length = 2;
					dir = 3;
					snake = { olc::vi2d(1, 8), olc::vi2d(2, 8) };
					apple = { olc::vi2d(13, 8) };
					snakeSpeed = (float)sWidth / 3.0f;

					almostDead = false;
					mode = 0;

					game = false;
					gameOver = false;
					menue = true;

					return true;
				}

				if (PointInRect(x, y, ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 75 + 2 * 8 * 6, (4 * 8 * 6), 8 * 6))
				{
					return false;
				}
			}
		}

		if (menue)
		{
			Clear(olc::BLACK);
			t += fElapsedTime / 5.0f;

			if (t > 1.0f)
			{
				t = fmod(t, 1.0f);
			}

			vector<string> title = {"S", "n", "a", "k", "e"};
			for (int i = 0; i < title.size(); i++)
			{
				DrawString(olc::vi2d(ScreenWidth() / 2 - (title.size() * 8 * 12) / 2 + i * 8 * 12, ScreenHeight() / 2 - 120), title[i], hsl2rgb(fmod(t + i * 1.0f / (float)title.size(), 1.0f), 1.0f, 0.5f), 12);
			}

			if (GetMouse(0).bPressed)
			{
				int x = GetMouseX(), y = GetMouseY();
				if (PointInRect(x, y, ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 30, (4 * 8 * 5), 8 * 6))
				{
					DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 5) / 2, ScreenHeight() / 2 + 30), "Play", olc::WHITE, 5);
					olc::SOUND::PlaySample(clickSound);
					game = true;
					menue = false;
				}

				if (PointInRect(x, y, ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 60 + 8 * 6, (4 * 8 * 5), 8 * 6))
				{
					return false;
				}
			}
			else 
			{
				DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 30), "Play", olc::WHITE, 6);
				DrawString(olc::vi2d(ScreenWidth() / 2 - (4 * 8 * 6) / 2, ScreenHeight() / 2 + 60 + 8 * 6), "Quit", olc::WHITE, 6);
			}
		}

		return true;
	}

	bool OnUserDestroy()
	{
		olc::SOUND::DestroyAudio();
		return true;
	}
};

bool Snake::bSynthPlaying = false;
float Snake::fSynthFrequency = 0.0f;
float Snake::fFilterVolume = 1.0f;
int Snake::nSamplePos = 0;
float Snake::fPreviousSamples[128];


int main()
{
	Snake demo;
	if (demo.Construct(1980, 1080, 1, 1, true))
		demo.Start();

	return 0;
}
