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

typedef std::wstring string;
typedef std::vector<int> layer;
typedef std::vector<layer> map;
typedef std::unordered_set<int> set;
typedef std::map<int, Gdiplus::Bitmap*> spritesheet;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static double playerx;
	static double playery;
	static bool direction[4];
	static bool ctrl;
	static Gdiplus::Bitmap* output;
	static map intmap;
	static layer hit;
	static spritesheet charsprite;
	static spritesheet sprite;
	static Gdiplus::Bitmap* overworld;
	static int mapwidth;
	static int mapheight;
    switch (message)
    {
	case WM_CREATE:
		{
			//static variable initializing
			{
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
				direction[0] = direction[1] = direction[2] = direction[3] = ctrl = false;
				output = new Gdiplus::Bitmap(256, 256); //initialize output (small viewing window)
				intmap = map(2, layer()); //perhaps I should add layers to the map as I parse the tmx file. But for now, just this for init
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
			}
			//parse the tmx file
			{ //TODO: make it add layers to the intmap as it encounters them in the tmx file
				std::wifstream tmx(L"Map1.tmx");
				string nextline;
				std::getline(tmx, nextline); //<?xml ...>
				std::getline(tmx, nextline); //<map ...>
				mapwidth = (int(nextline[int(nextline.find(L"width")) + 7]) - 0x30);
				for (int find = int(nextline.find(L"width")) + 8; nextline[find] != L'"'; ++find)
					(mapwidth *= 10) += (int(nextline[find]) - 0x30);
				mapheight = (int(nextline[int(nextline.find(L"height")) + 8]) - 0x30);
				for (int find = int(nextline.find(L"height")) + 9; nextline[find] != L'"'; ++find)
					(mapheight *= 10) += (int(nextline[find]) - 0x30);
				std::getline(tmx, nextline); // <tileset .../>
				std::getline(tmx, nextline); // <layer ...>
				std::getline(tmx, nextline); //  <data ...>
				std::getline(tmx, nextline); //first line of data
				while (nextline[0] != '<') { //lines of data
					for (string::iterator i = nextline.begin(); i != nextline.end(); ++i) if (*i != L',') {
						intmap[0].push_back(int(*i) - 0x30);
						for (++i; i != nextline.end() && *i != L','; ++i)
							(intmap[0].back() *= 10) += (int(*i) - 0x30);
						if (i == nextline.end()) break;
					}
					std::getline(tmx, nextline);
				} //</data>
				std::getline(tmx, nextline); // </layer>
				std::getline(tmx, nextline); // <layer ...>
				std::getline(tmx, nextline); //  <data ...>
				std::getline(tmx, nextline); //first line of data
				while (nextline[0] != L'<') {
					for (string::iterator i = nextline.begin(); i != nextline.end(); ++i) if (*i != L',') {
						intmap[1].push_back(int(*i) - 0x30);
						for (++i; i != nextline.end() && *i != L','; ++i)
							(intmap[1].back() *= 10) += (int(*i) - 0x30);
						if (i == nextline.end()) break;
					}
					std::getline(tmx, nextline);
				} //</data>
				std::getline(tmx, nextline); // </layer>
				std::getline(tmx, nextline); //</map>
			}
			//parse the .txt (hitbox) file and draw hit map
			{
				std::unordered_set<int> load;
				for (map::iterator m = intmap.begin(); m != intmap.end(); ++m)
					for (layer::iterator l = m->begin(); l != m->end(); ++l)
						if ((--(*l)) != -1) if (!load.count(*l))
							load.insert(*l);
				std::wifstream txt(L"OverworldHitboxes.txt");
				string nextline;
				std::map<int, layer> hitboxtmp;
				for (int i = 0; i < 40 * 36; ++i) {
					std::getline(txt, nextline);
					if (load.count(i)) {
						hitboxtmp[i] = layer();
						for (string::iterator j = nextline.begin(); j != nextline.end(); ++j)
							hitboxtmp[i].push_back(int(*j) - 0x30);
					}
				}
				hit = layer(mapwidth * 16 * mapheight * 16, 0); //initialize it with map output dimensions
				for (map::iterator z = intmap.begin(); z != intmap.end(); ++z)
					for (int tilex = 0; tilex < mapwidth; ++tilex)
						for (int tiley = 0; tiley < mapheight; ++tiley)
							if ((*z)[tilex + tiley * mapwidth] != -1)
								for (int x = 0; x < 16; ++x)
									for (int y = 0; y < 16; ++y)
										if (hitboxtmp[(*z)[tilex + tiley * mapwidth]][x + y * 16])
											hit[tilex * 16 + x + (tiley * 16 + y) * mapwidth * 16] = 1;
			}
			//load sprites
			{
				Gdiplus::Color c;
				Gdiplus::Bitmap* Spritesheet = Gdiplus::Bitmap::FromFile(L"Overworld.png"); //load the spritesheet for the overworld sprites
				for (map::iterator m = intmap.begin(); m != intmap.end(); ++m)
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
				overworld = new Gdiplus::Bitmap(mapwidth * 16, mapheight * 16); //initialize overworld (may need to be changed to "map" or something similar)
				Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(overworld); //get ready to draw the overworld
				for (map::iterator z = intmap.begin(); z != intmap.end(); ++z) //for each layer
					for (int x = 0; x < mapwidth; ++x) //go thru x
						for (int y = 0; y < mapheight; ++y) //go thru y
							if ((*z)[x + y * mapwidth] != -1) //ignore empty map locations
								g->DrawImage(sprite[(*z)[x + y * mapwidth]], x * 16, y * 16); //draw to overworld
				delete(g); //done drawing the overworld
			}
			//some misc window setup options
			{
				SetTimer(hWnd, 0, 20, TIMERPROC(NULL)); //T=20; 60fps would be T=16.66666
				DeleteMenu(GetMenu(hWnd), 1, MF_BYPOSITION);
			}
		}
		break;
	case WM_TIMER:
		{
			static int step = 0; ++step;
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
					for (int x = 0; x < 8 && movement; ++x) for (int y = 0; y < 8 && movement; ++y)
						if (hit[int(targetx) + x - 3 + (int(targety) + y - 3) * mapwidth * 16] == 1)
							movement = false;
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
				output = tmp.Clone(int(playerx) - 128, int(playery) - 128, 256, 256, PixelFormatDontCare);
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
                DestroyWindow(hWnd);
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
			g.DrawImage(output, 0, 0);
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
