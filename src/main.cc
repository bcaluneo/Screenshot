// Copyright (C) Brendan Caluneo

#include "windows.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include "SDL.h"
#include "SDL_syswm.h"

SDL_Window *window;
SDL_Renderer *render;
SDL_Texture *darkTexture, *lightTexture;

struct DisplayInfo {
  signed x, y, width, height;
  std::vector<unsigned> activeShot;

  void update() {
    activeShot.clear();
    x  = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    activeShot.resize(width*height);
  }
} DisplayInfo;

void grabScreen() {
  DisplayInfo.update();
  HDC dcScreen = GetDC(0);
  HDC dcTarget = CreateCompatibleDC(dcScreen);
  HBITMAP bmpTarget = CreateCompatibleBitmap(dcScreen, DisplayInfo.width, DisplayInfo.height);
  HGDIOBJ oldBmp = SelectObject(dcTarget, bmpTarget);
  BitBlt(dcTarget, 0, 0, DisplayInfo.width, DisplayInfo.height, dcScreen, DisplayInfo.x, DisplayInfo.y, SRCCOPY | CAPTUREBLT);
  GetBitmapBits(bmpTarget, DisplayInfo.width*DisplayInfo.height*4, static_cast<void *>(DisplayInfo.activeShot.data()));
  SelectObject(dcTarget, oldBmp);
  DeleteObject(bmpTarget);
  DeleteObject(oldBmp);
  DeleteDC(dcTarget);
  ReleaseDC(NULL, dcScreen);
}

void screenshot() {
  darkTexture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DisplayInfo.width, DisplayInfo.height);
  lightTexture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DisplayInfo.width, DisplayInfo.height);
  grabScreen();
  SDL_UpdateTexture(lightTexture, nullptr, DisplayInfo.activeShot.data(), 4*DisplayInfo.width);
  for (auto &col : DisplayInfo.activeShot) {
    col = (col & 0xfefefe) >> 1;
  }
  SDL_UpdateTexture(darkTexture, nullptr, DisplayInfo.activeShot.data(), 4*DisplayInfo.width);
  SDL_ShowWindow(window);
}

void snip(SDL_Rect clip) {
  SDL_HideWindow(window);
  if (clip.w < 0) {
    clip.w *=-1;
    clip.x -= clip.w;
  }

  if (clip.h < 0) {
    clip.h *=-1;
    clip.y -= clip.h;
  }

  std::vector<unsigned> pixels;
  for (int yp = 0; yp < clip.h; ++yp) {
    for (int xp = 0; xp < clip.w; ++xp) {
      auto col = DisplayInfo.activeShot[(xp+clip.x)+(yp+clip.y)*DisplayInfo.width];
      col = (col & 0xfefefe) << 1;
      pixels.push_back(col);
    }
  }

  HBITMAP bmp = CreateCompatibleBitmap(GetDC(NULL), clip.w, clip.h);
  SetBitmapBits(bmp, clip.w*clip.h*4, static_cast<void *>(pixels.data()));
  OpenClipboard(NULL);
  EmptyClipboard();
  SetClipboardData(CF_BITMAP, bmp);
  CloseClipboard();
  DeleteObject(bmp);
  SDL_DestroyTexture(darkTexture);
  SDL_DestroyTexture(lightTexture);
  pixels.clear();
  DisplayInfo.activeShot.clear();
}

// From: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// 0xa0 and 0xa1 are left and right shift.
// 0x11 is control
// 0xc0 is the ~ key
bool shiftDown = false, altDown = false;
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  KBDLLHOOKSTRUCT *info = (KBDLLHOOKSTRUCT*) lParam;
  auto k = info->vkCode;
  if (wParam == WM_KEYDOWN) {
    if (!shiftDown) shiftDown = 0x11/*k == 0xa0 || k == 0xa1*/;
    if (!altDown) altDown = 0x12/*k == 0x5B || k == 0x5C*/;
    if (k == 0xc0 && shiftDown) {
      screenshot();
      shiftDown = false;
    }
  } else if (wParam == WM_KEYUP) {
    shiftDown = altDown = 0;
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

SDL_Rect correct(SDL_Rect rect) {
  if (rect.w < 0) {
    rect.w *= -1;
    rect.x -= rect.w;
  }

  if (rect.h < 0) {
    rect.h *=-1;
    rect.y -= rect.h;
  }

  return { rect.x, rect.y, rect.w, rect.h };
}

int main(int argv, char** args) {
  SDL_Init(SDL_INIT_VIDEO);
  DisplayInfo.update();
  window = SDL_CreateWindow("Screenshot",
                            DisplayInfo.x,
                            DisplayInfo.y,
                            DisplayInfo.width,
                            DisplayInfo.height,
                            SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_SKIP_TASKBAR | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN);
  render = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

  HHOOK keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
  bool dragging = 0;
  SDL_Rect clippingRect { 0, 0, 0, 0 };

  SDL_Event event;
  for (bool quit = 0; !quit ;) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_MOUSEMOTION: {
            if (dragging) {
              clippingRect.w = event.button.x - clippingRect.x;
              clippingRect.h = event.button.y - clippingRect.y;
            }
					}
					break;
				case SDL_MOUSEBUTTONUP: {
            snip(clippingRect);
            clippingRect.x = clippingRect.y = clippingRect.w = clippingRect.h = -1;
            dragging = false;
          }
          break;
        case SDL_MOUSEBUTTONDOWN: {
            dragging = true;
            clippingRect.w = clippingRect.h = 1;
            clippingRect.x = event.button.x;
            clippingRect.y = event.button.y;
          }
          break;
        case SDL_QUIT:
          quit = 1;
          break;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            quit = 1;
          }

          break;
      }
    }

    SDL_Rect correctedRect = correct(clippingRect);
    SDL_RenderCopy(render, darkTexture, NULL, NULL);
    SDL_RenderCopy(render, lightTexture, &correctedRect, &correctedRect);
    SDL_SetRenderDrawColor(render, 0, 255, 0, 255);
    SDL_RenderDrawRect(render, &clippingRect);
    SDL_RenderPresent(render);
    SDL_Delay(25);
  }

	SDL_Quit();
  UnhookWindowsHookEx(keyboard);

  return 0;
}
