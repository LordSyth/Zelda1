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
#define DEBUG true
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
	static Gdiplus::Bitmap* output;
	static spritesheet sprite;
	static spritesheet charsprite;
	static Gdiplus::Bitmap* overworld;
	static Gdiplus::Bitmap* hitboxmap;
	static map intmap;
    switch (message)
    {
	case WM_CREATE:
		{
			playery = playerx = 512.1; //for now, spawn in center of map
			direction[0] = direction[1] = direction[2] = direction[3] = false;
			output = new Gdiplus::Bitmap(256, 256); //initialize output (small viewing window)
			intmap = map(2, layer()); //perhaps I should add layers to the map as I parse the tmx file. But for now, just this for init
			{ //character sprite loading
				Gdiplus::Bitmap* characterSpritesheet = Gdiplus::Bitmap::FromFile(L"character.png"); //load the spritesheet for the character sprites
				for (int x = 0; x < 4; ++x) for (int y = 0; y < 4; ++y) charsprite[x + y * 4] = characterSpritesheet->Clone(x * 16, y * 32, 16, 32, PixelFormatDontCare); //load character sprites into an array
				delete(characterSpritesheet); //done loading sprites => don't need source bitmap anymore
			} //done character sprite loading
			
			int mapwidth;
			int mapheight;
			{ //parse the tmx file
				std::wifstream tmx;
				tmx.open(L"Map1.tmx");
				string mapstr;
				std::getline(tmx, mapstr); //<?xml ...>
				std::getline(tmx, mapstr); //<map ...>
				mapwidth = (int(mapstr[int(mapstr.find(L"width")) + 7]) - 0x30);
				for (int find = int(mapstr.find(L"width")) + 8; mapstr[find] != L'"'; ++find)
					(mapwidth *= 10) += (int(mapstr[find]) - 0x30);
				mapheight = (int(mapstr[int(mapstr.find(L"height")) + 8]) - 0x30);
				for (int find = int(mapstr.find(L"height")) + 9; mapstr[find] != L'"'; ++find)
					(mapheight *= 10) += (int(mapstr[find]) - 0x30);
				std::getline(tmx, mapstr); // <tileset .../>
				std::getline(tmx, mapstr); // <layer ...>
				std::getline(tmx, mapstr); //  <data ...>
				std::getline(tmx, mapstr); //first line of data
				while (mapstr[0] != '<') { //lines of data
					for (string::iterator i = mapstr.begin(); i != mapstr.end(); ++i) if (*i != L',') {
						intmap[0].push_back(int(*i) - 0x30);
						for (++i; i != mapstr.end() && *i != L','; ++i)
							(intmap[0].back() *= 10) += (int(*i) - 0x30);
						if (i == mapstr.end()) break;
					}
					std::getline(tmx, mapstr);
				} //</data>
				std::getline(tmx, mapstr); // </layer>
				std::getline(tmx, mapstr); // <layer ...>
				std::getline(tmx, mapstr); //  <data ...>
				std::getline(tmx, mapstr); //first line of data
				while (mapstr[0] != L'<') {
					for (string::iterator i = mapstr.begin(); i != mapstr.end(); ++i) if (*i != L',') {
						intmap[1].push_back(int(*i) - 0x30);
						for (++i; i != mapstr.end() && *i != L','; ++i)
							(intmap[1].back() *= 10) += (int(*i) - 0x30);
						if (i == mapstr.end()) break;
					}
					std::getline(tmx, mapstr);
				} //</data>
				std::getline(tmx, mapstr); // </layer>
				std::getline(tmx, mapstr); //</map>
			} //done parsing the tmx file

			//check which sprites we need to load
			set loadsprites;
			for (map::iterator m = intmap.begin(); m != intmap.end(); ++m)
				for (layer::iterator l = m->begin(); l != m->end(); ++l)
					if (!loadsprites.count(--(*l)))
						loadsprites.insert(*l);

			//load the sprites we need
			Gdiplus::Bitmap* Spritesheet = Gdiplus::Bitmap::FromFile(L"Overworld.png"); //load the spritesheet for the overworld sprites
			for (set::iterator i = loadsprites.begin(); i != loadsprites.end(); ++i)
				if (*i != -1) if (!sprite.count(*i))
					sprite[*i] = Spritesheet->Clone(*i % 40 * 16, *i / 40 * 16, 16, 16, PixelFormatDontCare);
			delete(Spritesheet); //done loading sprites => don't need source bitmap anymore
			
			//load the hitbox sprites
			spritesheet hitboxsprite;
			Spritesheet = Gdiplus::Bitmap::FromFile(L"OverworldHitboxes.png");
			for (set::iterator i = loadsprites.begin(); i != loadsprites.end(); ++i)
				if ((*i) != -1) if (!hitboxsprite.count(*i))
					hitboxsprite[*i] = Spritesheet->Clone(*i % 40 * 16, *i / 40 * 16, 16, 16, PixelFormatDontCare);
			delete(Spritesheet);

			//draw overworld
			overworld = new Gdiplus::Bitmap(mapwidth * 16, mapheight * 16); //initialize overworld (may need to be changed to "map" or something similar)
			Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(overworld); //get ready to draw the overworld
			for (map::iterator z = intmap.begin(); z != intmap.end(); ++z) //for each layer
				for (int x = 0; x < mapwidth; ++x) //go thru x
					for (int y = 0; y < mapheight; ++y) //go thru y
						if ((*z)[x + y * mapwidth] != -1) //ignore empty map locations
							g->DrawImage(sprite[(*z)[x + y * mapwidth]], x * 21, y * 21); //draw to overworld
			delete(g); //done drawing the overworld

			//draw hitboxmap
			hitboxmap = new Gdiplus::Bitmap(mapwidth * 16, mapheight * 16); //initialize the bitmap
			g = Gdiplus::Graphics::FromImage(hitboxmap); //get ready to draw to the bitmap
			for (map::iterator z = intmap.begin(); z != intmap.end(); ++z) //for each layer
				for (int x = 0; x < mapwidth; ++x) //go thru x
					for (int y = 0; y < mapheight; ++y) //go thru y
						if ((*z)[x + y * mapwidth] != -1) //ignore empty map locations
							g->DrawImage(hitboxsprite[(*z)[x + y * mapwidth]], x * 21, y * 21); //draw to hitboxmap
			delete(g); //done drawing to the hitbox bitmap

			//some misc window setup options
			SetTimer(hWnd, 0, 20, TIMERPROC(NULL)); //T=20; 60fps would be T=16.66666
			DeleteMenu(GetMenu(hWnd), 1, MF_BYPOSITION);
		}
		break;
	case WM_TIMER:
		{
			static int step = 0; ++step;
			int dx = 0, dy = 0;
			if (direction[0]) ++dx;
			if (direction[1]) ++dy;
			if (direction[2]) --dx;
			if (direction[3]) --dy;

			bool movement = true;
			double targetx = playerx + (dx && dy) ? (0.709 * 2.0 * double(dx)) : (2.0 * double(dx));
			double targety = playery + (dx && dy) ? (0.709 * 2.0 * double(dy)) : (2.0 * double(dy));
			Gdiplus::Color c;
			for (int x = 0; x < 8 && movement; ++x) for (int y = 0; y < 8 && movement; ++y)
				if (hitboxmap->GetPixel(int(targetx) + x - 3, int(targety) + y - 3, &c) == Gdiplus::Status::Ok)
					if (c.GetValue() == Gdiplus::Color::Black)
						movement = false;
					else movement;
				else movement = false;
			if (movement) {
				playerx += (dx && dy) ? (0.709 * 2.0 * double(dx)) : (2.0 * double(dx));
				playery += (dx && dy) ? (0.709 * 2.0 * double(dy)) : (2.0 * double(dy));
			}

			int charspriteindex = 0;
			if (!dy && dx) { if (dx > 0) charspriteindex = 4; if (dx < 0) charspriteindex = 12; }
			else { if (dy > 0) charspriteindex = 0; if (dy < 0) charspriteindex = 8; }
			if (direction[0] || direction[1] || direction[2] || direction[3])
				charspriteindex += (step / 4) % 4;

			Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(output); //get ready to draw to our viewing window
			g->DrawImage(overworld, 128 - int(playerx), 128 - int(playery)); //draw overworld in a different location based on where the player is
			if (DEBUG) g->DrawImage(hitboxmap, 128 - int(playerx), 128 - int(playery));
			g->DrawImage(charsprite[charspriteindex], 128 - 7, 128 - 21); //draw the player's character
			delete(g); //done drawing to the drawing window
			InvalidateRect(hWnd, nullptr, false); //make sure to redraw
		}
		break;
	case WM_KEYDOWN:
		{
			if (wParam == VK_RIGHT || wParam == 0x44) direction[0] = true;
			if (wParam == VK_DOWN || wParam == 0x53) direction[1] = true;
			if (wParam == VK_LEFT || wParam == 0x41) direction[2] = true;
			if (wParam == VK_UP || wParam == 0x57) direction[3] = true;
		}
		break;
	case WM_KEYUP:
		{
			if (wParam == VK_RIGHT || wParam == 0x44) direction[0] = false;
			if (wParam == VK_DOWN || wParam == 0x53) direction[1] = false;
			if (wParam == VK_LEFT || wParam == 0x41) direction[2] = false;
			if (wParam == VK_UP || wParam == 0x57) direction[3] = false;
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
				delete(charsprite[i]);
			delete(overworld);
			PostQuitMessage(0);
		}
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
