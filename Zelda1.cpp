#include "stdafx.h"
#include "Zelda1.h"
#include <unordered_set>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <ObjIdl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
#define IDM_SAVE 200
HINSTANCE hInst;
WCHAR szTitle[100];
WCHAR szWindowClass[100];
ATOM MyRegisterClass(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	Gdiplus::GdiplusStartupInput input;
	ULONG_PTR token;
	Gdiplus::GdiplusStartup(&token, &input, nullptr);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, 100);
    LoadStringW(hInstance, IDC_ZELDA1, szWindowClass, 100);
    MyRegisterClass(hInstance);
    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ZELDA1));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	Gdiplus::GdiplusShutdown(token);
    return (int) msg.wParam;
}
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ZELDA1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ZELDA1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   hInst = hInstance;
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
   if (!hWnd) return FALSE;
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}

enum heart { empty = 0, quarter = 1, half = 2, threequarter = 3, full = 4 };
typedef std::wstring string;
typedef std::vector<int> layer;
typedef std::vector<layer> map;
typedef std::unordered_set<int> set;
typedef std::map<int, Gdiplus::Bitmap*> spritesheet;
typedef std::map<string, Gdiplus::Rect> objectgroup;

class area {
private:
	set contained;
	objectgroup zones;
public:
	map tiles;
	layer hit;
	int mapwidth;
	int mapheight;
	area() {}
	area(string); //assumed to be using tileset "Overworld.png", 40x36
	string ZoneCheck(int, int);
};
area::area(string filename) {
	//open file
	std::wifstream tmx(filename);
	string nextline;
	//grab mapsize
	{
		std::getline(tmx, nextline); //<?xml ...>
		std::getline(tmx, nextline); //<map ...>
		mapwidth = (int(nextline[int(nextline.find(L"width")) + 7]) - 0x30);
		for (int find = int(nextline.find(L"width")) + 8; nextline[find] != L'"'; ++find)
			(mapwidth *= 10) += (int(nextline[find]) - 0x30);
		mapheight = (int(nextline[int(nextline.find(L"height")) + 8]) - 0x30);
		for (int find = int(nextline.find(L"height")) + 9; nextline[find] != L'"'; ++find)
			(mapheight *= 10) += (int(nextline[find]) - 0x30);
	}
	//construct 'tiles'
	{
		std::getline(tmx, nextline); // <tileset .../>
		std::getline(tmx, nextline); //should be either <layer ...> or </map>
		while (nextline.find(L"<objectgroup") == string::npos) { //while we haven't found last layer,
			tiles.push_back(layer(mapwidth * mapheight)); //add another layer
			layer::iterator l = tiles.back().begin(); //make an iterator to the beginning of this new layer
			std::getline(tmx, nextline); //should be <data ...>
			std::getline(tmx, nextline); //should be the first line of actual data
			while (nextline != L"</data>") { //while we haven't found the end of this data,
				for (string::iterator s = nextline.begin(); s != nextline.end(); ++s) if (*s != L',') {
					(*l) = int(*s) - 0x30;
					for (++s; s != nextline.end() && *s != L','; ++s)
						((*l) *= 10) += (int(*s) - 0x30);
					++l;
					if (s == nextline.end()) break;
				}
				std::getline(tmx, nextline); //next line of data, or possibly </data>
			}
			std::getline(tmx, nextline); //should be </layer>
			std::getline(tmx, nextline); //should be either <objectgroup ...> or <layer ...>
		}
	}
	//construct 'zones'
	{
		std::getline(tmx, nextline); //should be first object
		while (nextline != L" </objectgroup>") {
			//int id = 0;
			//for (int find = int(nextline.find(L"id")) + 4; nextline[find] != L'"'; ++find)
			//	(id *= 10) += (nextline[find] - 0x30);
			string name = string();
			for (int find = int(nextline.find(L"name")) + 6; nextline[find] != L'"'; ++find)
				name += nextline[find];
			int x = 0;
			for (int find = int(nextline.find(L"x=")) + 3; nextline[find] != L'"'; ++find) {
				if (nextline[find] == L'-') ++find;
				(x *= 10) += (nextline[find] = 0x30);
			}
			if (nextline[nextline.find(L"x=") + 3] == L'-') x *= -1;
			int y = 0;
			for (int find = int(nextline.find(L"y=")) + 3; nextline[find] != L'"'; ++find) {
				if (nextline[find] == L'-') ++find;
				(x *= 10) += (nextline[find] = 0x30);
			}
			if (nextline[nextline.find(L"y=") + 3] == L'-') y *= -1;
			int width = 0;
			for (int find = int(nextline.find(L"width")) + 7; nextline[find] != L'"'; ++find)
				(width *= 10) += (nextline[find] - 0x30);
			int height = 0;
			for (int find = int(nextline.find(L"height")) + 8; nextline[find] != L'"'; ++find)
				(height *= 10) += (nextline[find] - 0x30);
			zones[name] = Gdiplus::Rect{ x, y, width, height };
			std::getline(tmx, nextline);
		}
		tmx.close();
	}
	//construct 'contained'
	{
		for (map::iterator m = tiles.begin(); m != tiles.end(); ++m)
			for (layer::iterator l = m->begin(); l != m->end(); ++l)
				if ((--(*l)) != -1) if (!contained.count(*l))
					contained.insert(*l);
	}
	//construct 'hit'
	{
		std::wifstream txt(L"OverworldHitboxes.txt");
		string nextline;
		std::map<int, layer> hitboxtmp;
		for (int i = 0; i < 40 * 36; ++i) {
			std::getline(txt, nextline);
			if (contained.count(i)) {
				hitboxtmp[i] = layer();
				for (string::iterator j = nextline.begin(); j != nextline.end(); ++j)
					hitboxtmp[i].push_back(int(*j) - 0x30);
			}
		}
		hit = layer(mapwidth * 16 * mapheight * 16, 0); //initialize it with map output dimensions
		for (map::iterator z = tiles.begin(); z != tiles.end(); ++z)
			for (int tilex = 0; tilex < mapwidth; ++tilex)
				for (int tiley = 0; tiley < mapheight; ++tiley)
					if ((*z)[tilex + tiley * mapwidth] != -1)
						for (int x = 0; x < 16; ++x)
							for (int y = 0; y < 16; ++y)
								if (hitboxtmp[(*z)[tilex + tiley * mapwidth]][x + y * 16])
									hit[tilex * 16 + x + (tiley * 16 + y) * mapwidth * 16] = 1;
	}
}
string area::ZoneCheck(int x, int y) {
	for (objectgroup::iterator o = zones.begin(); o != zones.end(); ++o) {
		if (
			x >= (*o).second.X &&
			x < (*o).second.Width &&
			y >= (*o).second.Y &&
			y < (*o).second.Height
			) {
			return (*o).first;
		}
	}
	return string();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static double playerx;
	static double playery;
	static int step;
	static bool direction[4];
	static bool ctrl;
	static Gdiplus::Bitmap* output;
	static spritesheet charsprite;
	static spritesheet sprite;
	static Gdiplus::Bitmap* overworld;
	static area* currentArea;
	static std::map<string, area> Areas;
	static std::vector<heart> health;
	static spritesheet hearts;
    switch (message)
    {
	case WM_CREATE:
		{
			//static variable initializing
			{
				Areas[string(L"Crossroads.tmx")] = area(string(L"Crossroads.tmx"));
				currentArea = &Areas[string(L"Crossroads.tmx")];
				std::wifstream save(L"save");
				if (save.is_open()) {
					string savestr;
					std::getline(save, savestr);
					save.close();
					playerx = playery = 0.0;
					string::iterator i;
					for (i = savestr.begin(); *i != L','; ++i)
						(playerx *= 10) += (*i - 0x30);
					for (++i; i != savestr.end(); ++i)
						(playery *= 10) += (*i - 0x30);
				}
				else playery = playerx = 512.0;
				step = 0;
				direction[0] = direction[1] = direction[2] = direction[3] = ctrl = false;
				output = new Gdiplus::Bitmap(256, 256); //initialize output (small viewing window)
				Gdiplus::Bitmap* characterSpritesheet = Gdiplus::Bitmap::FromFile(L"character.png"); //load the spritesheet for the character sprites
				Gdiplus::Color c;
				for (int x = 0; x < 4; ++x) for (int y = 0; y < 4; ++y) {
					//charsprite[x + y * 4] = characterSpritesheet->Clone(x * 16, y * 32, 16, 32, PixelFormatDontCare); //load character sprites into an array
					charsprite[x + y * 4] = new Gdiplus::Bitmap(16, 32);
					for (int px = 0; px < 16; ++px) for (int py = 0; py < 32; ++py) {
						characterSpritesheet->GetPixel(x * 16 + px, y * 32 + py, &c);
						charsprite[x + y * 4]->SetPixel(px, py, c);
					}
				}
				delete(characterSpritesheet); //done loading sprites => don't need source bitmap anymore
				Gdiplus::Bitmap* objectSpritesheet = Gdiplus::Bitmap::FromFile(L"objects.png"); //load the spritesheet containing the heart sprites
				for (int tilex = 0; tilex < 4; ++tilex) {
					hearts[tilex] = new Gdiplus::Bitmap(16, 16);
					for (int px = 0; px < 16; ++px) for (int py = 0; py < 16; ++py) {
						objectSpritesheet->GetPixel((8 - tilex) * 16 + px, py, &c);
						hearts[tilex]->SetPixel(px, py, c);
					}
				}
				delete(objectSpritesheet); //done loading sprites => don't need source bitmap anymore
			}
			//load sprites
			{
				Gdiplus::Color c;
				Gdiplus::Bitmap* Spritesheet = Gdiplus::Bitmap::FromFile(L"Overworld.png"); //load the spritesheet for the overworld sprites
				for (map::iterator m = currentArea->tiles.begin(); m != currentArea->tiles.end(); ++m)
					for (layer::iterator l = m->begin(); l != m->end(); ++l)
						if (*l != -1) if (!sprite[*l]) {
							//sprite[*l] = Spritesheet->Clone(*l % 40 * 16, *l / 40 * 16, 16, 16, PixelFormatDontCare);
							sprite[*l] = new Gdiplus::Bitmap(16, 16);
							for (int x = 0; x < 16; ++x)
								for (int y = 0; y < 16; ++y) {
									Spritesheet->GetPixel(*l % 40 * 16 + x, *l / 40 * 16 + y, &c);
									sprite[*l]->SetPixel(x, y, c);
								}
						}
				delete(Spritesheet); //done loading sprites => don't need source bitmap anymore
			}
			//draw overworld
			{
				overworld = new Gdiplus::Bitmap(currentArea->mapwidth * 16, currentArea->mapheight * 16); //initialize overworld (may need to be changed to "map" or something similar)
				Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(overworld); //get ready to draw the overworld
				for (map::iterator z = currentArea->tiles.begin(); z != currentArea->tiles.end(); ++z) //for each layer
					for (int x = 0; x < currentArea->mapwidth; ++x) //go thru x
						for (int y = 0; y < currentArea->mapheight; ++y) //go thru y
							if ((*z)[x + y * currentArea->mapwidth] != -1) //ignore empty map locations
								g->DrawImage(sprite[(*z)[x + y * currentArea->mapwidth]], x * 16, y * 16); //draw to overworld
				delete(g); //done drawing the overworld
			}
			//some misc window setup options
			{
				SetTimer(hWnd, 0, 20, TIMERPROC(NULL)); //T=20, or about 50fps; 60fps would be T=16.66666
				//DeleteMenu(GetMenu(hWnd), 1, MF_BYPOSITION); //remove that pesky "about" section
				InsertMenu(GetSubMenu(GetMenu(hWnd), 0), 1, MF_BYPOSITION, IDM_SAVE, L"Save	Ctrl+S");
			}
		}
		break;
	case WM_TIMER:
		{
			//switch (wParam);
			++step;
			int dx = 0, dy = 0;
			//decide movement
			{
				if (direction[0]) ++dx;
				if (direction[1]) ++dy;
				if (direction[2]) --dx;
				if (direction[3]) --dy;
				if (dx || dy) {
					bool movement = true;
					double targetx = playerx + 2.0 * double(dx) * ((dx && dy) ? 0.709 : 1.0);
					double targety = playery + 2.0 * double(dy) * ((dx && dy) ? 0.709 : 1.0);
					Gdiplus::Color c;
					for (int x = 0; x < 8 && movement; ++x) for (int y = 0; y < 8 && movement; ++y) {
						//first, check for any hitboxes we may be colliding with
						if (currentArea->hit[int(targetx) + x - 3 + (int(targety) + y - 3) * currentArea->mapwidth * 16] == 1)
							movement = false;
						//after, check for any zones we may have entered
						else {
							string s = currentArea->ZoneCheck(playerx, playery);
							if (s != string()) {
								//being here means we need to change areas
							}
						}
					}
					if (movement) {
						playerx += 2.0 * double(dx) * ((dx && dy) ? 0.709 : 1.0);
						playery += 2.0 * double(dy) * ((dx && dy) ? 0.709 : 1.0);
					}
				}
			}
			int charspriteindex = 0;
			//find the correct character sprite to use
			{
				if (!dy && dx) { if (dx > 0) charspriteindex = 4; if (dx < 0) charspriteindex = 12; }
				else { if (dy > 0) charspriteindex = 0; if (dy < 0) charspriteindex = 8; }
				if (direction[0] || direction[1] || direction[2] || direction[3])
					charspriteindex += (step / 4) % 4;
			}
			//draw
			{
				delete(output);
				Gdiplus::Bitmap tmp(overworld->GetWidth(), overworld->GetHeight());
				Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(&tmp);
				g->DrawImage(overworld, 0, 0);
				g->DrawImage(charsprite[charspriteindex], int(playerx) - 7, int(playery) - 23);
				delete(g);
				output = tmp.Clone(
					((int(playerx) - 128) < 0) ? 0 : (((int(playerx) - 128) > (currentArea->mapwidth * 16 - 256)) ? (currentArea->mapwidth * 16 - 256) : (int(playerx) - 128)),
					((int(playery) - 128) < 0) ? 0 : (((int(playery) - 128) > (currentArea->mapheight * 16 - 256)) ? (currentArea->mapheight * 16 - 256) : (int(playery) - 128)),
					256, 256, PixelFormatDontCare);
				InvalidateRect(hWnd, nullptr, false);
			}
		}
		break;
	case WM_KEYDOWN:
		{
			if (wParam == VK_RIGHT || wParam == 0x44) direction[0] = true;
			if (wParam == VK_DOWN || (wParam == 0x53 && !ctrl)) direction[1] = true;
			if (wParam == VK_LEFT || wParam == 0x41) direction[2] = true;
			if (wParam == VK_UP || wParam == 0x57) direction[3] = true;
			if (wParam == VK_CONTROL) ctrl = true;
			if (wParam == 0x53) if (ctrl) {
				std::wofstream save(L"save");
				save << int(playerx) << L',' << int(playery);
				save.close();
			}
		}
		break;
	case WM_KEYUP:
		{
			if (wParam == VK_RIGHT || wParam == 0x44) direction[0] = false;
			if (wParam == VK_DOWN || wParam == 0x53) direction[1] = false;
			if (wParam == VK_LEFT || wParam == 0x41) direction[2] = false;
			if (wParam == VK_UP || wParam == 0x57) direction[3] = false;
			if (wParam == VK_CONTROL) ctrl = false;
		}
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_EXIT:
				{
					DestroyWindow(hWnd);
				}
                break;
			case IDM_SAVE:
				{
					std::wofstream save(L"save");
					save << int(playerx) << L',' << int(playery);
					save.close();
				}
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			Gdiplus::Graphics g(hdc);
			Gdiplus::Rect R{ 0,0,512,512 };
			g.DrawImage(output, R);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		{
			delete(output);
			for (int i = 0; i < 40 * 36; ++i)
				if (sprite[i]) delete(sprite[i]);
			for (int i = 0; i < 4 * 4; ++i)
				if (charsprite[i]) delete(charsprite[i]);
			delete(overworld);
			PostQuitMessage(0);
		}
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
