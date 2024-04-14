#include<easyx.h>
#include<string>
#include<vector>
#include<windows.h>

const int window_width = 1280;
const int window_height = 720;

const int button_width = 192;
const int button_height = 75;

const double pi = 3.14159;


//��Ⱦ����
#pragma comment(lib,"MSIMG32.LIB")
#pragma comment(lib,"Winmm.lib")

bool is_game_started = false;
bool running = true;

inline void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//����Atlas��
class Atlas
{
public:
	
	Atlas(LPCTSTR path,int num)
	{
		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++)		
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	};

	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++)
		{
			delete frame_list[i];
		}
	};

public:
	std::vector<IMAGE*>frame_list;
};

Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;


//����Button��
class Button
{
public:
	Button(RECT rect,LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	{
		region = rect;
		
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);
	};

	~Button()=default;

	void ProcessEvent(const ExMessage&msg)
	{
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
			{
				status = Status::Hovered;
			}
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
			{
				status = Status::Idle;
			}
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursorHit(msg.x, msg.y))
			{
				status = Status::Pushed;
			}
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
			{
				OnClick();
				break;
			}
		default:
			break;
		}
	};

	void Draw()
	{
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed);
			break;
		}
	};

protected:
	//���м̳�Button����඼����ʵ���Լ���OnClick����
	virtual void OnClick() = 0;

private:
	//ö��Button��״̬
	enum class Status
	{
		Idle=0,
		Hovered,
		Pushed
	};


private:
	RECT region;
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status=Status::Idle;

private:
	//��������
	bool CheckCursorHit(int x, int y)
	{
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	};
};

//��ʼ��Ϸ��ť
class StartGameButton :public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}

	~StartGameButton() = default;

protected:
	void OnClick()
	{
		is_game_started = true;
	};
};

//�˳���Ϸ��ť
class QuitGameButton :public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}

	~QuitGameButton() = default;

protected:
	void OnClick()
	{
		running = false;
	};
};


//����Animation��
class Animation
{
public:
	Animation(Atlas* atlas, int interval)
	{
		interval_ms = interval;
		anim_atlas = atlas;
	};

	~Animation() = default;

	void play(int x, int y, int delta)
	{
		timer += delta;
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	};
private:
	int interval_ms = 0;
	int timer = 0;//��ʱ�� 
	int idx_frame = 0;//����֡��

private:
	Atlas* anim_atlas;

};

//����Bullet��
class Bullet
{
public:
	POINT position = { 0,0 };

public:
	Bullet() = default;
	~Bullet() = default;

	void draw() const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	};

private:
	const int RADIUS = 10;
};

//����Բ���ӵ�
class circle_bullet :public Bullet
{
public:
	void draw()const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	};

private:
	const int RADIUS = 10;
};

//���� enemy_bullet��
class enemy_bullet :public Bullet
{
public:

	double normalized_x;
	double normalized_y;

public:

	void set_x_and_y(double x, double y)
	{
		normalized_x = x;
		normalized_y = y;
	}

	void init_x_and_y(int x, int y)
	{
		position.x = x;
		position.y = y;
	}

	void draw()const
	{
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(255, 0, 0));
		fillcircle(position.x, position.y, RADIUS);
	};

private:
	const int RADIUS = 6;

};

//����Player��
class Player
{
public:
	Player()
	{
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left,45);
		anim_right = new Animation(atlas_player_right,45);
		init_bullet(bullet_list,circle_bullet_list);
	};

	~Player()
	{
		delete anim_left;
		delete anim_right;
	};

	void processevent(const ExMessage& msg)
	{
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
			case 'W':
				is_move_up = true;
				break;
			case 'A':
				is_move_left = true;
				break;
			case 'S':
				is_move_down = true;
				break;
			case 'D':
				is_move_right = true;
				break;
			default:
				break;
			}
			break;
		case WM_KEYUP:
			switch (msg.vkcode)
			{
			case 'W':
				is_move_up = false;
				break;
			case 'A':
				is_move_left = false;
				break;
			case 'S':
				is_move_down = false;
				break;
			case 'D':
				is_move_right = false;
				break;
			default:
				break;
			}
			break;
		}
	};

	void move()
	{
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			player_position.x += (int)(speed * normalized_x);
			player_position.y += (int)(speed * normalized_y);
		}
		if (player_position.x < 0)
		{
			player_position.x = 0;
		}
		if (player_position.y < 0)
		{
			player_position.y = 0;
		}
		if (player_position.x + frame_width > window_width)
		{
			player_position.x = window_width - frame_width;
		}
		if (player_position.y + frame_height > window_height)
		{
			player_position.y = window_height - frame_height;
		}
	};

	void draw(int delta)
	{
		int pos_shadow_x = player_position.x + (frame_width / 2 - shadow_width / 2);
		int pos_shadow_y = player_position.y + frame_height - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		static bool facing_left = false;
		int dir_x = is_move_right - is_move_left;
		if (dir_x < 0)
		{
			facing_left = true;
		}
		else if (dir_x > 0)
		{
			facing_left = false;
		}

		if (facing_left)
		{
			anim_left->play(player_position.x, player_position.y, delta);
		}
		else
		{
			anim_right->play(player_position.x, player_position.y, delta);
		}
	};

	const POINT& get_position() const
	{
		return player_position;
	}

	void init_bullet(std::vector<Bullet>& bullet_list,std::vector<circle_bullet>&circle_bullet_list)
	{
		for (int i = 0; i < 2; i++)
		{
			Bullet new_bullet;
			circle_bullet new_circle_bullet;
			bullet_list.push_back(new_bullet);
			circle_bullet_list.push_back(new_circle_bullet);
		}
	}

	void add_bullet(std::vector<Bullet>&bullet_list,std::vector<circle_bullet>&circle_bullet_list)
	{
		int i = rand() % 2;
		if (i == 0)
		{
			Bullet new_bullet;
			bullet_list.push_back(new_bullet);
		}
		else
		{
			circle_bullet new_bullet;
			circle_bullet_list.push_back(new_bullet);
		}
	}

public:
	const int speed = 3;
	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 32;
	const int window_width = 1280;
	const int window_height = 720;
	std::vector<Bullet> bullet_list;
	std::vector<circle_bullet> circle_bullet_list;
	int grade = 1;//����ȼ�

private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT player_position = { 500,500 };
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
	friend Bullet;
	friend circle_bullet;

};

//����Enemy��
class Enemy
{
public:
	Enemy() 
	{
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_right, 45);

		//�������ɱ߽�
		enum class SpawnEdge
		{
			up = 0,
			down,
			left,
			right
		};

		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		
		switch (edge)
		{
		case SpawnEdge::up:
			position.x = rand() % window_width;
			position.y = -frame_height;
			break;
		case SpawnEdge::down:
			position.x = rand() % window_width;
			position.y = window_height;
			break;
		case SpawnEdge::left:
			position.x = -frame_width;
			position.y = rand() % window_height;
			break;
		case SpawnEdge::right:
			position.x = window_width;
			position.y = rand() % window_height;
			break;
		default:
			break;
		}
	};

	bool check_bullet_collision(const Bullet& bullet)
	{
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + frame_width;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + frame_height;
		return is_overlap_x && is_overlap_y;
	}

	bool check_player_collision(const Player& player)
	{
		POINT check_position = { position.x + frame_width / 2,position.y + frame_height / 2 };
		POINT player_position = player.get_position();
		bool is_overlap_x = check_position.x >= player_position.x && check_position.x <= player_position.x + player.frame_width;
		bool is_overlap_y = check_position.y >= player_position.y && check_position.y <= player_position.y + player.frame_height;
		return is_overlap_x && is_overlap_y;
	}

	void move(const Player& player)
	{
		const POINT& player_position = player.get_position();
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			position.x += (int)(speed * normalized_x);
			position.y += (int)(speed * normalized_y);
		}

		if (dir_x < 0)
		{
			facing_left = true;
		}
		else if (dir_x > 0)
		{
			facing_left = false;
		}
	}

	void draw(int delta)
	{
		int pos_shadow_x = position.x + (frame_width / 2 - shadow_width / 2);
		int pos_shadow_y = position.y + frame_height - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		if (facing_left)
		{
			anim_left->play(position.x, position.y, delta);
		}
		else
		{
			anim_right->play(position.x, position.y, delta);
		}
	}

	~Enemy() 
	{
		delete anim_left;
		delete anim_right;
	};

	void hurt()
	{
		alive = false;
	};

	bool checkalive()
	{
		return alive;
	};

	//�����ӵ�
	void generate_bullet(Player& player)
	{
		POINT player_poition = player.get_position();
		int dir_x = position.x - player_poition.x;
		int dir_y = position.y - player_poition.y;
		double dir_len = sqrt(pow(dir_x, 2) + pow(dir_y, 2));
		enemy_bullet new_bullet;
		new_bullet.init_x_and_y(position.x, position.y);
		new_bullet.set_x_and_y(dir_x / dir_len, dir_y / dir_len);
		enemy_bullet_list.push_back(new_bullet);
	}

public:
	const int speed = 2;
	const int frame_width = 80;
	const int frame_height = 80;
	const int shadow_width = 48;
	std::vector<enemy_bullet>enemy_bullet_list;

private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0,0 };
	bool facing_left = false;
	bool alive = true;
	friend enemy_bullet;

};

//�����µ���
void generate_enemy(std::vector<Enemy*>& enemy_list)
{
	const int interval = 100;
	static int counter = 0;
	if ((++counter) % interval == 0)
	{
		enemy_list.push_back(new Enemy());
	}
}

//���������ӵ�λ��
void update_bullets(std::vector<Bullet>& bullet_list, const Player& player)
{
	const double radial_speed = 0.0045;//�����ٶ�
	const double tangent_speed = 0.0055;//�����ٶ�
	double radian_interval = 2 * pi / bullet_list.size();//�ӵ�֮��Ļ��ȼ��
	POINT player_position = player.get_position();
	double radius = 100 + 25 * sin(GetTickCount() * radial_speed);
	for (size_t i = 0; i < bullet_list.size(); i++)
	{
		double radian = GetTickCount() * tangent_speed + radian_interval * i;//��ǰ�ӵ����ڻ���
		bullet_list[i].position.x = player_position.x + player.frame_width / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.frame_height / 2 + (int)(radius * cos(radian));
	}
}

//����Բ���ӵ�λ��
void update_bullets(std::vector<circle_bullet>& circle_bullet_list, const Player& player)
{
	const double tangent_speed = 0.002;//�����ٶ�
	double radian_interval = 2 * pi / circle_bullet_list.size();//�ӵ��������
	POINT player_position = player.get_position();
	double radius = 100;
	for (size_t i = 0; i < circle_bullet_list.size(); i++)
	{
		double radian = GetTickCount() * tangent_speed + radian_interval * i;//�ӵ����ڻ���
		circle_bullet_list[i].position.x = player_position.x + player.frame_width / 2 + (int)(radius * sin(radian));
		circle_bullet_list[i].position.y = player_position.y + player.frame_height / 2 + (int)(radius * cos(radian));
	}
}

//���µ����ӵ�λ��
void update_enemy_bullet(std::vector<enemy_bullet>& enemy_bullet_list)
{
	const int speed = 0.5;
	for (size_t i = 0; i < enemy_bullet_list.size(); i++)
	{
		enemy_bullet_list[i].position.x += enemy_bullet_list[i].normalized_x * speed;
		enemy_bullet_list[i].position.y += enemy_bullet_list[i].normalized_y * speed;
	}
}

//������ҵ÷�
void draw_player_score(int score)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("��ǰ��ҵ÷�:%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

//������ҵȼ�
void draw_player_grade(int grade)
{
	static TCHAR text[64];
	_stprintf_s(text, _T("��ǰ��ҵȼ�:%d"), grade);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 30, text);
}

//������������ӵ��Ƿ�����
bool check_collition(Player &player,const enemy_bullet bullet)
{
	POINT player_position = player.get_position();
	bool is_overlap_x = bullet.position.x >= player_position.x && bullet.position.x <= player_position.x + player.frame_width;
	bool is_overlap_y = bullet.position.y >= player_position.y && bullet.position.y <= player_position.y + player.frame_height;
	return is_overlap_x && is_overlap_y;
}

int main()
{
	//��������
	initgraph(1280, 720);


	//�������ֲ�ȡ��
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);

	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 6);

	//�������
	int score = 0;
	ExMessage msg;
	IMAGE background;
	Player player;
	IMAGE img_menu;
	std::vector<Enemy*> enemy_list;




	RECT region_btn_start_game, region_btn_quit_game;

	region_btn_start_game.left = (window_width - button_width) / 2;
	region_btn_start_game.right = region_btn_start_game.left + button_width;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + button_height;

	region_btn_quit_game.left = (window_width - button_width) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + button_width;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + button_height;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));


	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&background, _T("img/background.jpg"));
	BeginBatchDraw();

	while (running)
	{
		DWORD begin_time = GetTickCount();

		while (peekmessage(&msg))
		{
			if (is_game_started)
			{
				player.processevent(msg);
			}
			else
			{
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}
		if (is_game_started)
		{
			player.move();
			
			//�����ӵ�λ�� 
			update_bullets(player.bullet_list, player);
			update_bullets(player.circle_bullet_list, player);
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				update_enemy_bullet(enemy_list[i]->enemy_bullet_list);
			}

			generate_enemy(enemy_list);
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				enemy_list[i]->move(player);
				enemy_list[i]->generate_bullet(player);
			}

			//�����Һ͵����ӵ�����
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				for (size_t j = 0; j < enemy_list[i]->enemy_bullet_list.size(); j++)
				{
					if (check_collition(player, enemy_list[i]->enemy_bullet_list[j]))
					{
						static TCHAR text[128];
						_stprintf_s(text, _T("���յ÷�:%d!"), score);
						MessageBox(GetHWnd(), text, _T("��Ϸ����"), MB_OK);
						running = false;
						break;
					}
				}
			}

			//���������ײ
			for (Enemy* enemy : enemy_list)
			{
				if (enemy->check_player_collision(player))
				{
					static TCHAR text[128];
					_stprintf_s(text, _T("���յ÷�:%d!"), score);
					MessageBox(GetHWnd(),text, _T("��Ϸ����"), MB_OK);
					running = false;
					break;
				}
			}

			//�����˺��ӵ���ײ
			for (Enemy* enemy : enemy_list)
			{
				for (const Bullet& bullet : player.bullet_list)
				{
					if (enemy->check_bullet_collision(bullet))
					{
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->hurt();
						score++;
						//������ҵȼ�
						if (score % 10 == 0)
						{
							player.grade++;
							player.add_bullet(player.bullet_list, player.circle_bullet_list);
						}
					}
				}
				for (const Bullet& bullet : player.circle_bullet_list)
				{
					if (enemy->check_bullet_collision(bullet))
					{
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->hurt();
						score++;
						//������ҵȼ�
						if (score % 10 == 0)
						{
							player.grade++;
							player.add_bullet(player.bullet_list, player.circle_bullet_list);
						}
					}
				}
			}

			//�������ֵΪ0�ĵ���
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->checkalive())
				{
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
				}
			}
		}

		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time-begin_time;
		if (delta_time < 1000 / 144)
		{
			Sleep(1000 / 144 - delta_time);
		}

		cleardevice();
		if (is_game_started)
		{
			putimage_alpha(0, 0, &background);
			player.draw(1000 / 144);
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				enemy_list[i]->draw(1000 / 144);
				for (size_t j = 0; j < enemy_list[i]->enemy_bullet_list.size(); j++)
				{
					enemy_list[i]->enemy_bullet_list[j].draw();
				}
			}
			for (const Bullet& bullet : player.bullet_list)
			{
				bullet.draw();
			}
			for (const circle_bullet& circle_bullet : player.circle_bullet_list)
			{
				circle_bullet.draw();
			}

			//������ҷ����͵ȼ�
			draw_player_score(score);
			draw_player_grade(player.grade);
		}
		else
		{
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}
		FlushBatchDraw();

	}

	delete atlas_player_left;
	delete atlas_player_right;


	EndBatchDraw();

	return 0;
}	