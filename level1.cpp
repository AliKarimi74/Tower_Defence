#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SCREEN_BPP = 32;
const int TOWER_BOARD_WIDTH = 50;
const int TOWER_BOARD_HEIGHT = 600;
const int SCORE_BOARD_WIDTH = 850;
const int SCORE_BOARD_HEIGHT = 50;

const int FRAMES_PER_SECOND = 30;

const int TILE_WIDTH = 20;
const int TILE_HEIGHT = 20;
const int total_tiles = (SCREEN_HEIGHT / TILE_HEIGHT) * (SCREEN_WIDTH / TILE_HEIGHT);

SDL_Surface *screen = NULL;
SDL_Surface *grass = NULL;
SDL_Surface *ground = NULL;
SDL_Surface *house = NULL;
const int GROUND_TOPRIGHT = 0;
const int GROUND_RIGHT = 1;
const int GROUND_BOTTOMRIGHT = 2;
const int GROUND_TOP = 3;
const int GROUND_CENTER = 4;
const int GROUND_BOTTOM = 5;
const int GROUND_TOPLEFT = 6;
const int GROUND_LEFT = 7;
const int GROUND_BOTTOMLEFT = 8;
SDL_Rect ground_clips[9];

SDL_Surface *devil = NULL;
const int DEVIL_TYPE = 0;
int enemy_width_heigh_speed_resistance_value[1][5] = { { 60, 60 , 1, 1, 20 } };

SDL_Surface *tower_place = NULL;
const int TOWER_PLACE_WIDTH = 40;
const int TOWER_PLACE_HEIGHT = 40;
SDL_Surface **towers_surface = new SDL_Surface*[1];
const int TOWER_WIDTH = 40;
const int TOWET_HEIGHT = 40;
int tower_radius_power_price_fireSpeed[1][4] = { { 80, 20, 100, 15 } };

SDL_Surface *input_page = NULL;
SDL_Surface *tower_board = NULL;
SDL_Surface *score_board = NULL;
SDL_Surface *score_surface = NULL;
SDL_Surface *money_surface = NULL;

TTF_Font* font = NULL;
TTF_Font *score_board_font = NULL;
TTF_Font *input_name_font = NULL;
SDL_Surface *massage = NULL;
SDL_Color textColor = { 181, 255, 18 };

Mix_Music *music = NULL;

SDL_Event event;

SDL_Surface *load_image(std::string, bool);
void apply_surface(int, int, SDL_Surface*, SDL_Surface*, SDL_Rect*);

class Timer
{
private:
	int startTicks;
	int pausedTicks;
	bool paused;
	bool started;

public:
	Timer();
	void start();
	void stop();
	void pause();
	void unpause();
	int get_ticks();
	bool is_started();
	bool is_paused();
};

Timer::Timer()
{
	startTicks = 0;
	pausedTicks = 0;
	paused = false;
	started = false;
}
void Timer::start()
{
	started = true;
	paused = false;
	startTicks = SDL_GetTicks();
}
void Timer::stop()
{
	started = false;
	paused = false;
}
void Timer::pause()
{
	if ((started == true) && (paused == false))
	{
		paused = true;
		pausedTicks = SDL_GetTicks() - startTicks;
	}
}
void Timer::unpause()
{
	if (paused == true)
	{
		paused = false;
		startTicks = SDL_GetTicks() - pausedTicks;
		pausedTicks = 0;
	}
}
int Timer::get_ticks()
{
	if (started == true)
	{
		if (paused == true)
		{
			return pausedTicks;
		}
		else
		{
			return SDL_GetTicks() - startTicks;
		}
	}
	return 0;
}
bool Timer::is_started() { return started; }
bool Timer::is_paused() { return paused; }

class Tile
{
private:
	SDL_Rect box;
	int type;
public:
	Tile(int x, int y, int tileType);
	Tile(int x, int y, int tileType, int, int);
	void show();
	int get_type();
	SDL_Rect get_box();
};

Tile::Tile(int x, int y, int tileType)
{
	box.x = x;
	box.y = y;
	box.w = TILE_WIDTH;
	box.h = TILE_HEIGHT;
	type = tileType;
}
Tile::Tile(int x, int y, int tileType, int w, int h)
{
	box.x = x;
	box.y = y;
	box.w = w;
	box.h = h;
	type = tileType;
}
void Tile::show()
{
	if (type == 0) { apply_surface(box.x, box.y, grass, screen, NULL); }
	else if (type > 0)
	{
		switch (type)
		{
		case 1: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_TOPRIGHT]); break;
		case 2: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_RIGHT]); break;
		case 3: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_BOTTOMRIGHT]); break;
		case 4: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_TOP]); break;
		case 5: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_CENTER]); break;
		case 6: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_BOTTOM]); break;
		case 7: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_TOPLEFT]); break;
		case 8: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_LEFT]); break;
		case 9: apply_surface(box.x, box.y, ground, screen, &ground_clips[GROUND_BOTTOMLEFT]); break;
		default:
			break;
		}
	}
}
int Tile::get_type() {return type;}
SDL_Rect Tile::get_box() {return box;}

class Map
{
private:
	std::string map_address;
	std::vector <int> enemy_course;
	std::vector <int> waves;
	std::vector <std::pair<int, int>> towers_place;
	std::pair<int, int> house_place;
public:
	Map();
	bool set_map(Tile*[]);
	int handle_event();
	void show_house();
	int get_numberOfTiles();
	std::vector <int> get_enemyCourse();
	std::vector <int> get_waves();
	std::vector <std::pair<int, int>> get_towers_place();
	std::pair<int, int> get_house_place();
};

Map::Map()
{
	map_address = "level1.map";
}
bool Map::set_map(Tile *tiles[])
{
	std::ifstream map(map_address);
	if (map.fail()) return false;
	int X = 0, Y = 0, tile_type;
	for (int i = 0; i < total_tiles; i++)
	{
		tile_type = -1;
		map >> tile_type;
		if (tile_type < 0)
		{
			map.close();
			return false;
		}
		if (tile_type == 99)
		{
			house_place.first = X;
			house_place.second = Y;
			tiles[i] = new Tile(X, Y, 0);
		}
		else tiles[i] = new Tile(X, Y, tile_type);
		X += TILE_WIDTH;
		if (X == SCREEN_WIDTH)
		{
			X = 0;
			Y += TILE_HEIGHT;
		}
	}
	int part_of_course;
	map >> part_of_course;
	int a_course;
	for (int i = 0; i < part_of_course*3; i++)
	{
		map >> a_course;
		enemy_course.push_back(a_course);
	}
	int number_of_waves;
	map >> number_of_waves;
	int temp;
	for (int i = 0; i < number_of_waves * 2; i++)
	{
		map >> temp;
		waves.push_back(temp);
	}
	int number_of_tower_place;
	map >> number_of_tower_place;
	std::pair <int, int> temp1;
	for (int i = 0; i < number_of_tower_place; i++)
	{
		map >> temp1.first;
		map >> temp1.second;
		temp1.first = (temp1.first - 1) * TILE_HEIGHT;
		temp1.second = (temp1.second - 1) * TILE_WIDTH;
		towers_place.push_back(temp1);
	}
	map.close();
	return true;
}
int Map::handle_event()
{
	int x, y;
	int index_of_tower = -1;
	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			x = event.button.x;
			y = event.button.y;
			for (int i = 0; i < towers_place.size(); i++)
			{
				if (x >= towers_place[i].second && x <= towers_place[i].second + TOWER_PLACE_WIDTH && y >= towers_place[i].first && y <= towers_place[i].first + TOWER_PLACE_HEIGHT)
				{
					index_of_tower = i;
					break;
				}
			}
		}
	}
	return index_of_tower;
}
void Map::show_house()
{
	apply_surface(house_place.first, house_place.second, house, screen, NULL);
}
std::vector <int> Map::get_enemyCourse() { return enemy_course; }
std::vector <int> Map::get_waves() { return waves; }
std::vector <std::pair<int, int>> Map::get_towers_place() { return towers_place; }
std::pair<int, int> Map::get_house_place() { return house_place; }

class Enemy
{
private:
	int x, y;
	int enemy_type;
	int health;
	int speed;
	int enemy_value;
	int resistance;
	int enemy_in_what_course;
	SDL_Rect health_bar_back;
	SDL_Rect health_bar_front;
public:
	Enemy(int x0, int y0, int type, int sp0, int val, int resis);
	~Enemy();
	bool move(std::vector <int> enemy_course);
	void show();
	bool demage(int amountOfDemage);
	int get_x();
	int get_y();
	int get_health();
	int get_type();
	int get_value();
	int get_resistance();
};

Enemy::Enemy(int x0, int y0, int type, int sp0, int val, int resis)
{
	x = x0;
	y = y0;
	enemy_type = type;
	health = 100;
	speed = sp0;
	enemy_value = val;
	resistance = resis;
	enemy_in_what_course = 0;
}
Enemy::~Enemy()
{
	speed = 0;
	health = 0;
}
bool Enemy::move(std::vector <int> enemy_course)
{
	int x_Vel, y_Vel;
	int destination[3];
	for (int i = 0; i < 3; i++) destination[i] = enemy_course[(enemy_in_what_course*3) + i];
	switch (destination[2])
	{
	case 1: x_Vel = -speed; y_Vel = 0; break;
	case 3: x_Vel = 0; y_Vel = -speed; break;
	case 5: x_Vel = speed; y_Vel = 0; break;
	case 7: x_Vel = 0; y_Vel = speed; break;
	default:
		break;
	}
	x += x_Vel;
	y += y_Vel;
	health_bar_back.x = health_bar_front.x = x + enemy_width_heigh_speed_resistance_value[enemy_type][0] / 6;
	health_bar_back.y = health_bar_front.y = y - enemy_width_heigh_speed_resistance_value[enemy_type][1] / 12;
	health_bar_back.w = (enemy_width_heigh_speed_resistance_value[enemy_type][0] / 6) * 4;
	health_bar_front.w = health * (enemy_width_heigh_speed_resistance_value[enemy_type][0] / 6) * 0.04;
	health_bar_back.h = health_bar_front.h = enemy_width_heigh_speed_resistance_value[enemy_type][1] / 12;
	if (y == (destination[0]-1) * TILE_WIDTH && x == (destination[1]-1) * TILE_HEIGHT)
	{
		enemy_in_what_course++;
		if (enemy_in_what_course == enemy_course.size()/3) return true;
	}
	return false;
}
void Enemy::show()
{
	apply_surface(x, y, devil, screen, NULL);
	SDL_FillRect(screen, &health_bar_back, SDL_MapRGB(screen->format, 0, 0, 0));
	SDL_FillRect(screen, &health_bar_front, SDL_MapRGB(screen->format, 0, 250, 0));
}
bool Enemy::demage(int amountOfDemage)
{
	if (health > 1 ) health += amountOfDemage;
	else return true;
	return false;
}
int Enemy::get_x() {return x;}
int Enemy::get_y() { return y; }
int Enemy::get_health() { return health; }
int Enemy::get_type() { return enemy_type; }
int Enemy::get_value() { return enemy_value; }
int Enemy::get_resistance() { return resistance; }

class Tower_Board
{
private:
	std::vector <int> towers;
	int active_tower_index;
	SDL_Rect active_rect;
public:
	Tower_Board();
	void add_tower(int);
	void handle_event();
	void show();
	int get_active_tower_index();
};

Tower_Board::Tower_Board()
{
	active_tower_index = -1;
}
void Tower_Board::add_tower(int tower_type)
{
	towers.push_back(tower_type);
}
void Tower_Board::handle_event()
{
	if (event.type == SDL_KEYDOWN)
	{
		switch (event.key.keysym.sym)
		{
		case SDLK_1:
		{
					   if (active_tower_index != 0) active_tower_index = 0;
					   else active_tower_index = -1;
					   break;
		}
		default:
			break;
		}
	}
}
void Tower_Board::show()
{
	int x = SCREEN_WIDTH + 5, y = 0;
	std::stringstream num, price;
	apply_surface(SCREEN_WIDTH, 0, tower_board, screen, NULL);
	for (int i = 0; i < towers.size(); i++)
	{
		if (active_tower_index == i)
		{
			active_rect.x = x - 5;
			active_rect.y = y;
			active_rect.w = 50;
			active_rect.h = 75;
			SDL_FillRect(screen, &active_rect, SDL_MapRGB(screen->format, 254, 255, 42));
		}
		SDL_SetAlpha(towers_surface[i], SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
		apply_surface(x, y, towers_surface[i], screen, NULL);
		num << "Press " << i + 1;
		massage = TTF_RenderText_Solid(font, num.str().c_str(), textColor);
		y += 40;
		apply_surface(SCREEN_WIDTH + ((TOWER_BOARD_WIDTH - massage->w) / 2), y, massage, screen, NULL);
		y += 15;
		price << tower_radius_power_price_fireSpeed[towers[i]][2] << " $";
		massage = TTF_RenderText_Solid(font, price.str().c_str(), textColor);
		apply_surface(SCREEN_WIDTH + ((TOWER_BOARD_WIDTH - massage->w) / 2), y, massage, screen, NULL);
		y += 20;
	}
}
int Tower_Board::get_active_tower_index() { return active_tower_index; }

class Score_Board
{
private:
	int money;
	int score;
public:
	Score_Board(int primal_money, int primal_score);
	void set_money(int change);
	void set_score(int change);
	void show();
	int get_money();
	int get_score();
};

Score_Board::Score_Board(int primal_money, int primal_score)
{
	money = primal_money;
	score = primal_score;
}
void Score_Board::set_money(int change)
{
	money += change;
}
void Score_Board::set_score(int change)
{
	score += change;
}
void Score_Board::show()
{
	std::stringstream temp;
	std::stringstream temp1;
	apply_surface(0, SCREEN_HEIGHT, score_board, screen, NULL);
	apply_surface(0, SCREEN_HEIGHT, score_surface, screen, NULL);
	temp << "  " << score;
	massage = TTF_RenderText_Solid(score_board_font, temp.str().c_str(), textColor);
	apply_surface(50, SCREEN_HEIGHT + (SCORE_BOARD_HEIGHT - massage->h) / 2, massage, screen, NULL);
	apply_surface(SCORE_BOARD_WIDTH / 4, SCREEN_HEIGHT, money_surface, screen, NULL);
	temp1 << "  " << money << " $";
	massage = TTF_RenderText_Solid(score_board_font, temp1.str().c_str(), textColor);
	apply_surface((SCORE_BOARD_WIDTH / 4) + 50, SCREEN_HEIGHT + (SCORE_BOARD_HEIGHT - massage->h) / 2, massage, screen, NULL);
}
int Score_Board::get_money() { return money; }
int Score_Board::get_score() { return score; }

class Player
{
private:
	std::string name;
	int money;
	int score;
	SDL_Surface *text;
public:
	Player();
	void input_name();
	void change_score(int amount_of_change);
	void change_money(int amount_of_change);
	int get_money();
	int get_score();
};

Player::Player()
{
	name = " ";
	text = NULL;
	SDL_EnableUNICODE(SDL_ENABLE);
}
void Player::input_name()
{
	bool name_Entered = false, show_name = false;
	while (!name_Entered)
	{
		apply_surface(0, 0, input_page, screen, NULL);
		massage = TTF_RenderText_Solid(input_name_font, "Enter your name: ", textColor);
		apply_surface((SCREEN_WIDTH + TOWER_BOARD_WIDTH - massage->w) / 2, (SCREEN_HEIGHT + SCORE_BOARD_HEIGHT) / 3, massage, screen, NULL);
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				std::string temp = name;
				if (name.length() <= 16)
				{
					if (event.key.keysym.unicode == (Uint16)' ') name += (char)event.key.keysym.unicode;
					else if ((event.key.keysym.unicode >= (Uint16)'0') && (event.key.keysym.unicode <= (Uint16)'9')) name += (char)event.key.keysym.unicode;
					else if ((event.key.keysym.unicode >= (Uint16)'A') && (event.key.keysym.unicode <= (Uint16)'Z')) name += (char)event.key.keysym.unicode;
					else if ((event.key.keysym.unicode >= (Uint16)'a') && (event.key.keysym.unicode <= (Uint16)'z')) name += (char)event.key.keysym.unicode;
				}
				if ((event.key.keysym.sym == SDLK_BACKSPACE) && (name.length() > 1)) name.erase(name.length() - 1);
				if (event.key.keysym.sym == SDLK_RETURN) name_Entered = true;
				if (name != temp)
				{
					SDL_FreeSurface(text);
					text = TTF_RenderText_Solid(input_name_font, name.c_str(), textColor);
					show_name = true;
				}
			}
		}
		if (show_name) apply_surface((SCREEN_WIDTH + TOWER_BOARD_WIDTH - text->w) / 2, 2 * (SCREEN_HEIGHT + SCORE_BOARD_HEIGHT) / 3, text, screen, NULL);
		SDL_Flip(screen);
	}
	std::stringstream welc;
	welc << "WELCOME " << name;
	apply_surface(0, 0, input_page, screen, NULL);
	text = TTF_RenderText_Solid(input_name_font, welc.str().c_str(), textColor);
	apply_surface((SCREEN_WIDTH + TOWER_BOARD_WIDTH - text->w) / 2, (SCREEN_HEIGHT + SCORE_BOARD_HEIGHT) / 3, text, screen, NULL);
	SDL_Flip(screen);
	SDL_Delay(2000);
	SDL_EnableUNICODE(SDL_DISABLE);
}
void Player::change_score(int amount_of_change) { score += amount_of_change; }
void Player::change_money(int amount_of_change){ money += amount_of_change; }
int Player::get_money() { return money; }
int Player::get_score() { return score; }

class Tower
{
private:
	int x, y;
	int type;
	int grade;
	int energy;
	int radius;
	int power;
	bool isFire;
	int price;
	int alpha;
	int alpha_state;
	int changeAlphaAmount;
public:
	Tower(int, int, int);
	bool check_fire(Enemy);
	void change_alpha();
	void fire(Enemy);
	void show();
	int get_x();
	int get_y();
	bool get_isFire();
	int get_power();
};

Tower::Tower(int X, int Y, int tower_type)
{
	x = X;
	y = Y;
	type = tower_type;
	grade = 0;
	energy = 100;
	radius = tower_radius_power_price_fireSpeed[type][0];
	power = tower_radius_power_price_fireSpeed[type][1];
	isFire = false;
	price = tower_radius_power_price_fireSpeed[type][2];
	alpha = SDL_ALPHA_OPAQUE;
	alpha_state = 0;
	changeAlphaAmount = tower_radius_power_price_fireSpeed[type][3];
}
bool Tower::check_fire(Enemy enemy)
{
	int enemy_x, enemy_y;
	int tower_x = x + TOWER_WIDTH / 2, tower_y = y + TOWET_HEIGHT / 2;
	int distance;
	enemy_x = enemy.get_x() + enemy_width_heigh_speed_resistance_value[enemy.get_type()][0] / 2;
	enemy_y = enemy.get_y() + enemy_width_heigh_speed_resistance_value[enemy.get_type()][1] / 2;
	distance = sqrt(pow(tower_x - enemy_x, 2) + pow(tower_y - enemy_y, 2));
	if (distance <= radius)
	{
		isFire = true;
		return true;
	}
	isFire = false;
	return false;
}
void Tower::change_alpha()
{
	if (isFire)
	{
		if (alpha == SDL_ALPHA_OPAQUE)
		{
			alpha_state = -1;
			isFire = false;
		}
		if (alpha <= SDL_ALPHA_TRANSPARENT + changeAlphaAmount) alpha_state = 1;
		alpha += alpha_state * changeAlphaAmount;
	}
}
void Tower::fire(Enemy a_enemy)
{
}
void Tower::show()
{
	if (!isFire) SDL_SetAlpha(towers_surface[type], SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
	else SDL_SetAlpha(towers_surface[type], SDL_SRCALPHA, alpha);
	apply_surface(x, y, towers_surface[type], screen, NULL);
}
int Tower::get_x() { return x; }
int Tower::get_y() { return y; }
bool Tower::get_isFire() { return isFire; }
int Tower::get_power() { return power; }


void show_towers_place(std::vector <std::pair<int, int>> tower_places)
{
	for (int i = 0; i < tower_places.size(); i++) apply_surface(tower_places[i].second, tower_places[i].first, tower_place, screen, NULL);
}

std::vector<Enemy> build_enemy_wave(int number_of_enemy, int enemy_type, int direction, int start_point_x, int start_point_y)
{
	std::vector<Enemy> enemy_wave;
	int enemy_width = enemy_width_heigh_speed_resistance_value[enemy_type][0], enemy_height = enemy_width_heigh_speed_resistance_value[enemy_type][1], enemy_speed = enemy_width_heigh_speed_resistance_value[enemy_type][2], enemy_resistance = enemy_width_heigh_speed_resistance_value[enemy_type][3], enemy_value = enemy_width_heigh_speed_resistance_value[enemy_type][4];
	int X = start_point_x, Y = start_point_y;
	int x_increase, y_increase;
	switch (direction)
	{
	case 1: x_increase = enemy_width; y_increase = 0; break;
	case 3: x_increase = 0; y_increase = enemy_height; break;
	case 5: x_increase = -enemy_width; y_increase = 0; break;
	case 7: x_increase = 0; y_increase = -enemy_height; break;
	default:
		break;
	}
	for (int i = 0; i < number_of_enemy; i++)
	{
		Enemy a_enemy(X, Y, enemy_type, enemy_speed, enemy_value, enemy_resistance);
		enemy_wave.push_back(a_enemy);
		X += x_increase;
		Y += y_increase;
	}
	return enemy_wave;
}

SDL_Surface *load_image(std::string filename, bool Mapping = true)
{
	SDL_Surface* loadedImage = IMG_Load(filename.c_str());;
	SDL_Surface* optimizedImage = NULL;
	if (loadedImage != NULL)
	{
		optimizedImage = SDL_DisplayFormat(loadedImage);
		SDL_FreeSurface(loadedImage);
		if (optimizedImage != NULL && Mapping)
		{
			SDL_SetColorKey(optimizedImage, SDL_SRCCOLORKEY, SDL_MapRGB(optimizedImage->format, 0xFF, 0xFF, 0xFF));
		}
	}
	return optimizedImage;
}

void apply_surface(int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* clip = NULL)
{
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

bool init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {return false;}
	screen = SDL_SetVideoMode(SCREEN_WIDTH + TOWER_BOARD_WIDTH, SCREEN_HEIGHT + SCORE_BOARD_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
	if (TTF_Init() == -1) return false;
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) {return false;}
	if (screen == NULL) {return false;}
	SDL_WM_SetCaption("ToWeR DeFeNsE", NULL);
	return true;
}

bool load_files()
{
	grass = load_image("Files/Pic/grass.png");
	ground = load_image("Files/Pic/ground.png");
	devil = load_image("Files/Pic/devil.png");
	tower_place = load_image("Files/Pic/tower_place.png");
	towers_surface[0] = load_image("Files/Pic/tower.png");
	tower_board = load_image("Files/Pic/tower_board.png", false);
	score_board = load_image("Files/Pic/score_board.png", false);
	score_surface = load_image("Files/Pic/score.png");
	money_surface = load_image("Files/Pic/money.png");
	input_page = load_image("Files/Pic/input_page.png");
	house = load_image("Files/Pic/house.png");
	if (house == NULL || input_page == NULL || score_surface == NULL || money_surface == NULL || ground == NULL || grass == NULL || devil == NULL || tower_place == NULL || towers_surface[0] == NULL || tower_board == NULL || score_board == NULL) return false;
	font = TTF_OpenFont("Files/Font/micross.ttf", 10);
	score_board_font = TTF_OpenFont("Files/Font/ITCKRIST.TTF", 20);
	input_name_font = TTF_OpenFont("Files/Font/RAVIE.TTF", 50);
	if (input_name_font == NULL || score_board_font == NULL || font == NULL) return false;
	music = Mix_LoadMUS("Files/Space 1990.wav");
	if (music == NULL) {return false;}
	return true;
}

void clean_up(Tile *tiles[])
{
	SDL_FreeSurface(grass);
	SDL_FreeSurface(ground);
	SDL_FreeSurface(devil);
	SDL_FreeSurface(tower_place);
	SDL_FreeSurface(towers_surface[0]);
	SDL_FreeSurface(tower_board);
	SDL_FreeSurface(score_board);
	SDL_FreeSurface(score_surface);
	SDL_FreeSurface(money_surface);
	SDL_FreeSurface(input_page);
	SDL_FreeSurface(house);
	for (int i = 0; i < total_tiles; i++) delete tiles[i];
	TTF_CloseFont(font);
	TTF_CloseFont(score_board_font);
	TTF_CloseFont(input_name_font);
	Mix_FreeMusic(music);
	Mix_CloseAudio();
	TTF_Quit();
	SDL_Quit();
}

void set_clips(SDL_Rect clips[])
{
	int row, column;
	for (int i = 0; i < 9; i++)
	{
		column = i % 3;
		row = i / 3;
		clips[i].x = row * TILE_WIDTH;
		clips[i].y = column * TILE_HEIGHT;
		clips[i].w = TILE_WIDTH;
		clips[i].h = TILE_HEIGHT;	
	}
}


int main(int argc, char* args[])
{
	bool quit = false;
	if (init() == false) {return 1;}
	if (load_files() == false) {return 1;}
	set_clips(ground_clips);
	Player player;
	player.input_name();
	Map level1;
	Tile *tiles[total_tiles];
	Timer fps;
	if (level1.set_map(tiles) == false) { return 1; }
	std::vector <int>  waves = level1.get_waves();
	std::vector <std::pair<int, int>> towers_places = level1.get_towers_place();
	Tower_Board tower_Board;
	tower_Board.add_tower(0);
	Score_Board ScoreBorad(500, 0);
	std::vector <Tower> towers;
	bool create_tower = true;
	bool FIRE = false, killedEnemy = false;
	int number_of_wave = 0;
	std::vector<Enemy> wave = build_enemy_wave(waves[number_of_wave], waves[number_of_wave + 1], 1, 830, 420);

	while (quit == false)
	{
		if (Mix_PlayingMusic() == 0) {Mix_PlayMusic(music, -1) == -1;}
		fps.start();
		int insert_tower = -1;
		while (SDL_PollEvent(&event))
		{
			insert_tower = level1.handle_event();
			tower_Board.handle_event();
			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
			}
			if (event.type == SDL_QUIT) {quit = true;}
		}
		for (int i = 0; i < wave.size(); i++) wave[i].move(level1.get_enemyCourse());
		if (insert_tower != -1 && tower_Board.get_active_tower_index() != -1 && ScoreBorad.get_money() >= tower_radius_power_price_fireSpeed[tower_Board.get_active_tower_index()][2])
		{
			create_tower = true;
			for (int i = 0; i < towers.size(); i++)
			{
				if (towers[i].get_x() == towers_places[insert_tower].second && towers[i].get_y() == towers_places[insert_tower].first)
				{
					create_tower = false;
					break;
				}
			}
			if (create_tower)
			{
				Tower temp_tower(towers_places[insert_tower].second, towers_places[insert_tower].first, tower_Board.get_active_tower_index());
				towers.push_back(temp_tower);
				ScoreBorad.set_money(-tower_radius_power_price_fireSpeed[tower_Board.get_active_tower_index()][2]);
			}
		}
		for (int i = 0; i < towers.size(); i++)
		{
			for (int j = 0; j < wave.size(); j++)
			{
				FIRE = towers[i].check_fire(wave[j]);
				if (FIRE)
				{
					towers[i].change_alpha();
					if (!towers[i].get_isFire())
					{
						killedEnemy = wave[j].demage(-(towers[i].get_power() / wave[j].get_resistance()));
						if (killedEnemy) wave.erase(wave.begin()+j);
					}
					break;
				}
			}
		}
		if (wave.empty())
		{
			number_of_wave += 2;
			if (waves.size() <= number_of_wave)
			{
				SDL_Delay(500);
				massage = TTF_RenderText_Solid(input_name_font, "<<   Y O U  W I N   >>", textColor);
				apply_surface(0, 0, input_page, screen, NULL);
				apply_surface((SCREEN_WIDTH + TOWER_BOARD_WIDTH - massage->w) / 2, (SCREEN_HEIGHT + SCORE_BOARD_HEIGHT - massage->h) / 2, massage, screen, NULL);
				SDL_Flip(screen);
				SDL_Delay(5000);
				quit = true;
			}
			else wave = build_enemy_wave(waves[number_of_wave], waves[number_of_wave + 1], 1, 830, 420);
		}
		if (wave[0].get_x() == level1.get_house_place().first - enemy_width_heigh_speed_resistance_value[wave[0].get_type()][0])
		{
			SDL_Delay(500);
			massage = TTF_RenderText_Solid(input_name_font, "<<   Y O U  L O S E   >>", textColor);
			apply_surface(0, 0, input_page, screen, NULL);
			apply_surface((SCREEN_WIDTH + TOWER_BOARD_WIDTH - massage->w) / 2, (SCREEN_HEIGHT + SCORE_BOARD_HEIGHT - massage->h) / 2, massage, screen, NULL);
			SDL_Flip(screen);
			SDL_Delay(5000);
			quit = true;
		}
		for (int i = 0; i < total_tiles; i++) tiles[i]->show();
		show_towers_place(level1.get_towers_place());
		for (int i = 0; i < wave.size(); i++) wave[i].show();
		for (int i = 0; i < towers.size(); i++) towers[i].show();
		level1.show_house();
		tower_Board.show();
		ScoreBorad.show();
		SDL_Flip(screen);
		if (fps.get_ticks() < 1000 / FRAMES_PER_SECOND) {SDL_Delay((1000 / FRAMES_PER_SECOND) - fps.get_ticks());}
	}
	clean_up(tiles);
	return 0;
}
