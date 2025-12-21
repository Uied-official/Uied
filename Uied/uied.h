#ifndef UIED_H
#define UIED_H

#include <winapifamily.h>

#ifndef _WINDOWS_
#define _WINDOWS_
#include <sdkddkver.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#endif

#include <vector>
#include <string>
#include <functional>

namespace uied {

    enum class Type { BUTTON, LABEL, INPUT, TERMINAL };

    struct Element {
        Type type;
        std::string text; 
        std::vector<std::string> history; 
        int x, y, w, h;
        COLORREF color;
        std::function<void(std::string)> onCommand;
        bool isFocused = false;
    };

    class Engine {
    private:
        std::vector<Element> elements;
        HWND hwnd;
        int focusedIdx = -1;

    public:
        static Engine& getInstance() { static Engine inst; return inst; }

        size_t add(Element e) { elements.push_back(e); return elements.size() - 1; }

        void print(int id, std::string line) {
            if (id >= 0 && id < elements.size()) {
                elements[id].history.push_back(line);
                if (elements[id].history.size() > 20) elements[id].history.erase(elements[id].history.begin());
                refresh();
            }
        }

        void clear(int id) {
            if (id >= 0 && id < elements.size()) {
                elements[id].history.clear();
                refresh();
            }
        }

        void draw(HDC hdc) {
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_REGULAR, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Consolas");
            SelectObject(hdc, hFont);

            for (auto& e : elements) {
                RECT r = { e.x, e.y, e.x + e.w, e.y + e.h };

                if (e.type == Type::TERMINAL) {
                    HBRUSH bg = CreateSolidBrush(RGB(10, 10, 10));
                    FillRect(hdc, &r, bg);
                    DeleteObject(bg);

                    if (e.isFocused) {
                        HBRUSH border = CreateSolidBrush(RGB(0, 255, 0));
                        FrameRect(hdc, &r, border);
                        DeleteObject(border);
                    }

                    SetTextColor(hdc, RGB(0, 255, 0)); 
                    SetBkMode(hdc, TRANSPARENT);

                    int offset = 5;
                    for (const auto& line : e.history) {
                        TextOutA(hdc, e.x + 5, e.y + offset, line.c_str(), (int)line.length());
                        offset += 18;
                    }

                    std::string inputLine = "> " + e.text + (e.isFocused ? "_" : "");
                    TextOutA(hdc, e.x + 5, e.y + e.h - 20, inputLine.c_str(), (int)inputLine.length());
                }
                else if (e.type == Type::BUTTON) {
                    HBRUSH br = CreateSolidBrush(e.color);
                    FillRect(hdc, &r, br);
                    SetTextColor(hdc, RGB(255,255,255));
                    DrawTextA(hdc, e.text.c_str(), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    DeleteObject(br);
                }
            }
            DeleteObject(hFont);
        }

        void onChar(char c) {
            if (focusedIdx != -1) {
                auto& e = elements[focusedIdx];
                if (c == 8) { // Backspace
                    if (!e.text.empty()) e.text.pop_back();
                }
                else if (c == 13) { // Enter
                    if (e.type == Type::TERMINAL && e.onCommand) {
                        e.history.push_back("> " + e.text);
                        e.onCommand(e.text);
                        e.text = "";
                    }
                }
                else if (c >= 32) {
                    e.text += c;
                }
                refresh();
            }
        }

        void onMouseDown(int mx, int my) {
            focusedIdx = -1;
            for (int i = 0; i < elements.size(); ++i) {
                elements[i].isFocused = false;
                if (mx >= elements[i].x && mx <= elements[i].x + elements[i].w &&
                    my >= elements[i].y && my <= elements[i].y + elements[i].h) {
                    focusedIdx = i;
                    elements[i].isFocused = true;
                }
            }
            refresh();
        }

        void refresh() { InvalidateRect(hwnd, NULL, FALSE); }
        void setHwnd(HWND h) { hwnd = h; }
    };

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_CHAR: Engine::getInstance().onChar((char)wp); break;
        case WM_LBUTTONDOWN: Engine::getInstance().onMouseDown(LOWORD(lp), HIWORD(lp)); break;
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            HDC mdc = CreateCompatibleDC(hdc);
            RECT rc; GetClientRect(hwnd, &rc);
            HBITMAP mbmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mdc, mbmp);
            FillRect(mdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
            Engine::getInstance().draw(mdc);
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mdc, 0, 0, SRCCOPY);
            DeleteObject(mbmp); DeleteDC(mdc); EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    inline int terminal(int x, int y, int w, int h, std::function<void(std::string)> cmdFunc) {
        return (int)Engine::getInstance().add({Type::TERMINAL, "", {}, x, y, w, h, 0, cmdFunc});
    }

    inline void init(int w, int h, const char* title) {
        HINSTANCE hi = GetModuleHandle(NULL);
        WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hi; wc.lpszClassName = "UIED_TERM";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
        HWND hwnd = CreateWindowEx(0, "UIED_TERM", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, w, h, NULL, NULL, hi, NULL);
        Engine::getInstance().setHwnd(hwnd);
        MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
}
#endif

// 2025 UIED