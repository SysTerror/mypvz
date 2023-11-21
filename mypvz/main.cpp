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
	int type; // ��ǰѡ��ֲ�0 is δѡ�С�1��2��3���� is �� n ��ֲ��
	int frameIndex; // ��ǰ����֡���
};

struct plant map[5][9];

struct sunball
{
	int x, y;
	int frameIndex; // ��ǰ����֡���
	int destY; // �������Ŀ�����
	bool used; // �Ƿ����
	int timer; // �ƴμ�ʱ��
	float xoff; // �ռ�ʱÿ��λʱ�� x ��ƫ����
	float yoff; // �ռ�ʱÿ��λʱ�� y ��ƫ����

};

struct sunball pool[10]; // �������

int curX, curY; // ��ǰѡ�п��ơ��������ʱ������
int curPlant; // ��ǰѡ��ֲ�0 is δѡ�С�1��2��3���� is �� n ��ֲ��
int sunshine; // ����ֵ

bool fileExist(const char* name) // �ж��ļ��Ƿ����
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
		// ���ؿ���ͼ��
		sprintf_s(name, sizeof(name), "res/seeds/seed-%d.png", i + 1);
		loadimage(&imgCard[i], name, 50, 70); // ���Ƴߴ�

		// ����ֲ��ͼ��
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
	curPlant = 0; // ��ʼ����ǰֲ�δѡ��
	sunshine = 150; // ��ʼ������ֵ

	// ����������ͼ��
	memset(pool, 0, sizeof(pool));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunball/%d.png", i + 1);
		loadimage(&imgSunball[i], name);
	}

	// �����������
	srand(time(NULL));

    initgraph(WIN_WIDTH, WIN_HEIGHT, 1); // 1 ����������̨����

	// ��������
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 25; // �ָ�
	f.lfWidth = 10; // �ֿ�
	strcpy(f.lfFaceName, "Segoe UI"); // ��������
	f.lfQuality = ANTIALIASED_QUALITY; // �����
	settextstyle(&f);
	setbkmode(TRANSPARENT); // ����͸��
	setcolor(BLACK); // ������ɫ
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
				map[i][j].frameIndex++; // ֲ���������������֡ǰ��
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
	// �ƴμ�ʱ����������ѭ��������ÿָ��ʱ��ִ�� 1 �Σ��ʼƴμ���ʱ
	static int count = 0;
	static int fre = 50;
	count++;
	if (count >= fre)
	{
		// ����������� fre �ͼƴα� count
		fre = 50 + rand() % 51; // fre ȡֵ��Χ��[50, 100]
		count = 0;

		// ���������������ȡ���� 1 �����õ�������
		int sunballMax = sizeof(pool) / sizeof(pool[0]);
		int i;
		for (i = 0; i < sunballMax && pool[i].used; i++);
		if (i >= sunballMax) return; // ���ӿ���
	
		// �趨����������
		pool[i].used = true;
		pool[i].frameIndex = 0;
		pool[i].x = 296 + rand() % (8 * 80); // x ȡֵ��Χ��[256 + col / 2, 256 + 8.5 * col - 1]
		pool[i].y = 0;
		pool[i].destY = 85 + (rand() % 5) * 100; // destY ȡֵ��Χ��[85, 85 + 4 * row], �ҽ��������� row �ɵõ�ֵ
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
			pool[i].frameIndex = (pool[i].frameIndex + 1) % 29; // �������������������֡ǰ��
			if (pool[i].timer == 0)
			{
				pool[i].y += 5; // ���������䶯��������֡ǰ��
			}
			if (pool[i].y >= pool[i].destY) // ���������
			{
				pool[i].timer++; // �����ƴμ�ʱ��
				if (pool[i].timer > 100)
				{
					pool[i].used = false; // ����������
				}
			}
		}
		else if (pool[i].xoff) // ���ú�ִ���ռ�����
		{
			pool[i].x -= pool[i].xoff;
			pool[i].y -= pool[i].yoff;

			// ����ÿ��λʱ�� x��y ��ƫ����
			// ѭ��ִ�У�ʹÿ���������ʱ���¼��㣬���׿�������ת����������������ۼ�
			// �ռ�ϻ�������� (200, 0)
			float destX = 200;
			float destY = 0;
			float theta = atan((pool[i].y - destY) / (pool[i].x - destX)); // ʼĩ������ˮƽƫ�� theta
			float loff = 30; // ʼĩ�������ϵĵ�λƫ���� loff
			pool[i].xoff = loff * cos(theta);
			pool[i].yoff = loff * sin(theta);

			if (pool[i].x <= destX || pool[i].y <= destY)
			{
				pool[i].xoff = 0;
				pool[i].yoff = 0;
				sunshine += UNIT_SUNBALL; // ����ռ�
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

				// ����ÿ��λʱ�� x��y ��ƫ����
				// �ռ�ϻ�������� (200, 0)
				float destX = 200;
				float destY = 0;
				float theta = atan((y - destY) / (x - destX)); // ʼĩ������ˮƽƫ�� theta
				float loff = 30; // ʼĩ�������ϵĵ�λƫ���� loff
				pool[i].xoff = loff * cos(theta);
				pool[i].yoff = loff * sin(theta);
			}
		}
	}
}

void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) // ��̽�����Ϣ
	{
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x > 278 && msg.x < 278 + 51 * PLANTS_COUNT && msg.y > 8 && msg.y < 78)
			{
				// ��ȡѡ�п������
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
			printf("px(%d, %d)\n", msg.x, msg.y); // ��������귨������
		}
	}
}

void updateGame() {
	updatePlant();
	createSunball();
	updateSunball();
}

void updateWin() {
	//˫���塤����������ʱ��˸
	BeginBatchDraw();//��ʼ˫����

    putimage(0, 0, &imgBg); // ��Ⱦ����ͼ
    drawAlpha(200, 0, &imgBar); // ��Ⱦ������

	char pointText[8];
	sprintf_s(pointText, sizeof(pointText), "%d", sunshine);
	outtextxy(222, 58, pointText); // ��Ⱦ������ֵ�ı�

	for (int i = 0; i < PLANTS_COUNT; i++)
	{
		int x = 278 + i * 51;
		int y = 8;
		drawAlpha(x, y, &imgCard[i]); // ��Ⱦ������
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
				drawAlpha(x, y, imgPlant[plantType][plantFrameIndex]); // ��Ⱦ����������ֲ��
			}
		}
	}

	if (curPlant > 0)
	{
		IMAGE* img = imgPlant[curPlant - 1][0];
		int curPX = curX - img->getwidth() / 2;
		int curPY = curY - img->getheight() / 2;
		drawAlpha(curPX, curPY, img); //��Ⱦ����������ƶ��Ĵ���ֲ��
	}

	int sunballMax = sizeof(pool) / sizeof(pool[0]);
	for (int i = 0; i < sunballMax; i++)
	{
		if (pool[i].used || pool[i].xoff) // �������Ƿ�ȡ�ã�������Ⱦ��������Ⱦ
		{
			IMAGE* img = &imgSunball[pool[i].frameIndex];
			drawAlpha(pool[i].x, pool[i].y, img); // ��Ⱦ��������
		}
	}

	// ��Ⱦ������ͼ��
	//drawAlpha(200, 0, &imgSunball[0]);

	EndBatchDraw();//����˫����
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
		if (timer > 80) // ���θ���֮���ۼƳ���ָ��ʱ����flag ��ǲ�Ϊ�� 
		{
			flag = true;
			timer = 0;
		}
		if (flag) // ��� flag ���Ϊ�棬ִ��һ����Ϸ����
		{
			flag = false;
			updateGame(); // ����Ҳ���������˳������
		}
		updateWin(); // ��Ҳ���������˿������
	}
    system("pause");
    return 0;
}