#include <graphics.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "png.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define WIN_WIDTH   1020
#define WIN_HEIGHT  600

#define UNIT_SUNBALL 25

enum {PEA, SUNFLOWER, PLANTS_COUNT}; // CHERRY, WALLNUT, POTATO, SNOW_PEA, CHOMPER, 

IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCard[PLANTS_COUNT];
IMAGE* imgPlant[PLANTS_COUNT][20];
IMAGE imgSunball[29];

struct plant
{
	int type; // 当前选中植物·0 is 未选中·1、2、3…… is 第 n 种植物
	int frameIndex; // 当前序列帧序号
};

struct plant map[5][9];

struct sunball
{
	int x, y;
	int frameIndex; // 当前序列帧序号
	int destY; // 阳光球的目标落点
	bool used; // 是否出池
	int timer; // 计次计时器
	float xoff; // 收集时每单位时长 x 的偏移量
	float yoff; // 收集时每单位时长 y 的偏移量

};

struct sunball pool[10]; // 阳光球池

int curX, curY; // 当前选中卡牌·跟随鼠标时的坐标
int curPlant; // 当前选中植物·0 is 未选中·1、2、3…… is 第 n 种植物
int sunshine; // 阳光值

bool fileExist(const char* name) // 判断文件是否存在
{
	FILE* fp = fopen(name, "r");
	if (fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

void gameInit() {
    loadimage(&imgBg, "res/images/background1.jpg");
    loadimage(&imgBar, "res/images/SeedBank.png");
	memset(imgPlant, 0, sizeof(imgPlant));
	memset(map, 0, sizeof(map));
	char name[64];
	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		// 加载卡牌图像
		sprintf_s(name, sizeof(name), "res/seeds/seed-%d.png", i + 1);
		loadimage(&imgCard[i], name, 50, 70); // 卡牌尺寸

		// 加载植物图像
		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/plants/%d/%d.png", i, j + 1);
			if (fileExist(name))
			{
				imgPlant[i][j] = new IMAGE;
				loadimage(imgPlant[i][j], name);
			}
			else
			{
				break;
			}
		}
	}
	curPlant = 0; // 初始·当前植物·未选中
	sunshine = 150; // 初始·阳光值

	// 加载阳光球图像
	memset(pool, 0, sizeof(pool));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunball/%d.png", i + 1);
		loadimage(&imgSunball[i], name);
	}

	// 配置随机种子
	srand(time(NULL));

    initgraph(WIN_WIDTH, WIN_HEIGHT, 1); // 1 代表保留控制台界面

	// 设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 25; // 字高
	f.lfWidth = 10; // 字宽
	strcpy(f.lfFaceName, "Segoe UI"); // 字体名称
	f.lfQuality = ANTIALIASED_QUALITY; // 抗锯齿
	settextstyle(&f);
	setbkmode(TRANSPARENT); // 背景透明
	setcolor(BLACK); // 字体颜色
}

void startUI() {
	IMAGE imgBg, imgMenu, imgMenuP;
	loadimage(&imgBg, "res/ui/menu.png");
	loadimage(&imgMenu, "res/ui/menu1.png");
	loadimage(&imgMenuP, "res/ui/menu2.png");
	int flag = 0;
	while (true)
	{
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		drawAlpha(475, 75, flag ? &imgMenuP : &imgMenu);
		ExMessage msg;
		if (peekmessage(&msg))
		{
			if (msg.x > 475 && msg.x < 805 &&
				msg.y > 75 && msg.y < 215)
			{
				flag = 1;
			}
			else
			{
				flag = 0;
			}
			if (msg.message == WM_LBUTTONUP && flag)
			{
				return;
			}
		}
		EndBatchDraw();
	}
}

void updatePlant() {
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				map[i][j].frameIndex++; // 植物待机动画·序列帧前进
				int plantType = map[i][j].type - 1;
				int plantFrameIndex = map[i][j].frameIndex;
				if (imgPlant[plantType][plantFrameIndex] == NULL)
				{
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

void createSunball() {
	// 计次计时器，由下文循环本函数每指定时长执行 1 次，故计次即计时
	static int count = 0;
	static int fre = 50;
	count++;
	if (count >= fre)
	{
		// 重置随机次数 fre 和计次标 count
		fre = 50 + rand() % 51; // fre 取值范围：[50, 100]
		count = 0;

		// 从阳光球池中依次取出第 1 个可用的阳光球
		int sunballMax = sizeof(pool) / sizeof(pool[0]);
		int i;
		for (i = 0; i < sunballMax && pool[i].used; i++);
		if (i >= sunballMax) return; // 池子空了
	
		// 设定阳光球属性
		pool[i].used = true;
		pool[i].frameIndex = 0;
		pool[i].x = 296 + rand() % (8 * 80); // x 取值范围：[256 + col / 2, 256 + 8.5 * col - 1]
		pool[i].y = 0;
		pool[i].destY = 85 + (rand() % 5) * 100; // destY 取值范围：[85, 85 + 4 * row], 且仅包括代入 row 可得的值
		pool[i].timer = 0;
		pool[i].xoff = 0;
		pool[i].yoff = 0;
	}
}

void updateSunball() {
	int sunballMax = sizeof(pool) / sizeof(pool[0]);
	for (int i = 0; i < sunballMax; i++)
	{
		if (pool[i].used)
		{
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // 阳光球待机动画·序列帧前进
			if (pool[i].timer == 0)
			{
				pool[i].y += 5; // 阳光球下落动画·序列帧前进
			}
			if (pool[i].y >= pool[i].destY) // 阳光球落地
			{
				pool[i].timer++; // 启动计次计时器
				if (pool[i].timer > 100)
				{
					pool[i].used = false; // 弃用阳光球
				}
			}
		}
		else if (pool[i].xoff) // 弃用后执行收集动画
		{
			pool[i].x -= pool[i].xoff;
			pool[i].y -= pool[i].yoff;

			// 设置每单位时长 x、y 的偏移量
			// 循环执行，使每次坐标更新时重新计算，以拮抗浮点数转换整数带来的误差累计
			// 收集匣中心坐标 (200, 0)
			float destX = 200;
			float destY = 0;
			float theta = atan((pool[i].y - destY) / (pool[i].x - destX)); // 始末点连线水平偏角 theta
			float loff = 30; // 始末点连线上的单位偏移量 loff
			pool[i].xoff = loff * cos(theta);
			pool[i].yoff = loff * sin(theta);

			if (pool[i].x <= destX || pool[i].y <= destY)
			{
				pool[i].xoff = 0;
				pool[i].yoff = 0;
				sunshine += UNIT_SUNBALL; // 完成收集
			}
		}
	}
}

void collectSunshine(ExMessage* msg) {
	int countSunball = sizeof(pool) / sizeof(pool[0]);
	int w = imgSunball[0].getwidth();
	int h = imgSunball[0].getheight();
	for (int i = 0; i < countSunball; i++)
	{
		if (pool[i].used)
		{
			int x = pool[i].x;
			int y = pool[i].y;
			if (msg->x > x && msg->x < x + w && msg->y > y && msg->y < y + h)
			{
				pool[i].used = false;
				mciSendString("play res/sunshine.mp3", 0, 0, 0);

				// 设置每单位时长 x、y 的偏移量
				// 收集匣中心坐标 (200, 0)
				float destX = 200;
				float destY = 0;
				float theta = atan((y - destY) / (x - destX)); // 始末点连线水平偏角 theta
				float loff = 30; // 始末点连线上的单位偏移量 loff
				pool[i].xoff = loff * cos(theta);
				pool[i].yoff = loff * sin(theta);
			}
		}
	}
}

void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) // 嗅探鼠标消息
	{
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x > 278 && msg.x < 278 + 51 * PLANTS_COUNT && msg.y > 8 && msg.y < 78)
			{
				// 获取选中卡牌序号
				int index = (msg.x - 278) / 51;
				curPlant = index + 1;

				status = 1;
				curX = msg.x;
				curY = msg.y;
			}
			else if (msg.x > 256 && msg.y > 90 && status == 1)
			{
				int row = (msg.y - 90) / 100;
				int col = (msg.x - 256) / 80;
				if (map[row][col].type == 0)
				{
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
				}
				curPlant = 0;
				status = 0;
			}
			else
			{
				curPlant = 0;
				status = 0;
			}
			collectSunshine(&msg);
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP && status == 1)
		{
			if (msg.x > 256 && msg.y > 90)
			{
				int row = (msg.y - 90) / 100;
				int col = (msg.x - 256) / 80;
				if (map[row][col].type == 0)
				{
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
				}
				curPlant = 0;
				status = 0;
			}
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			printf("px(%d, %d)\n", msg.x, msg.y); // 土法·鼠标法测坐标
		}
	}
}

void updateGame() {
	updatePlant();
	createSunball();
	updateSunball();
}

void updateWin() {
	//双缓冲·解决画面更新时闪烁
	BeginBatchDraw();//开始双缓冲

    putimage(0, 0, &imgBg); // 渲染·地图
    drawAlpha(200, 0, &imgBar); // 渲染·卡槽

	char pointText[8];
	sprintf_s(pointText, sizeof(pointText), "%d", sunshine);
	outtextxy(222, 58, pointText); // 渲染·阳光值文本

	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		int x = 278 + i * 51;
		int y = 8;
		drawAlpha(x, y, &imgCard[i]); // 渲染·卡牌
	}

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				int x = 256 + j * 80;
				int y = 90 + i * 100;
				int plantType = map[i][j].type - 1;
				int plantFrameIndex = map[i][j].frameIndex;
				drawAlpha(x, y, imgPlant[plantType][plantFrameIndex]); // 渲染·种下来的植物
			}
		}
	}

	if (curPlant > 0)
	{
		IMAGE* img = imgPlant[curPlant - 1][0];
		int curPX = curX - img->getwidth() / 2;
		int curPY = curY - img->getheight() / 2;
		drawAlpha(curPX, curPY, img); //渲染·跟随鼠标移动的待命植物
	}

	int sunballMax = sizeof(pool) / sizeof(pool[0]);
	for (int i = 0; i < sunballMax; i++)
	{
		if (pool[i].used || pool[i].xoff) // 阳光球是否被取用，是则渲染，否则不渲染
		{
			IMAGE* img = &imgSunball[pool[i].frameIndex];
			drawAlpha(pool[i].x, pool[i].y, img); // 渲染·阳光球
		}
	}

	// 渲染·测试图像
	//drawAlpha(200, 0, &imgSunball[0]);

	EndBatchDraw();//结束双缓冲
}


int main() {
    gameInit();
	startUI();
	int timer = 0;
	bool flag = true;
	while (true)
	{
		userClick();
		timer += getDelay();
		if (timer > 80) // 两次更新之间累计超过指定时长，flag 标记才为真 
		{
			flag = true;
			timer = 0;
		}
		if (flag) // 如果 flag 标记为真，执行一次游戏更新
		{
			flag = false;
			updateGame(); // 非玩家操作反馈，顺滑养眼
		}
		updateWin(); // 玩家操作反馈，丝滑跟手
	}
    system("pause");
    return 0;
}