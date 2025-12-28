#ifndef UIED_H
#define UIED_H

#define NOMINMAX
#include <windows.h>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

namespace uied {

    enum class Type { BUTTON, LABEL, INPUT, TERMINAL, SLIDER };

    struct Element {
        Type type;
        std::string text;
        std::vector<std::string> history;
        int x, y, w, h;
        COLORREF color;
        std::function<void()> onClick;
        std::function<void(std::string)> onCommand;
        std::function<void(float)> onSlide;
        float sliderVal = 0.0f;
        bool isFocused = false;
        bool isHovered = false;
        bool isDragging = false;
    };

    class Engine {
    private:
        std::vector<Element> elements;
        HWND hwnd;
        int focusedIdx = -1;

    public:
        static Engine& getInstance() { static Engine inst; return inst; }

        size_t add(Element e) { elements.push_back(e); return elements.size() - 1; }

        std::string getInput(int id) { 
            return (id >= 0 && id < (int)elements.size()) ? elements[id].text : ""; 
        }

        void print(int id, std::string line) {
            if (id >= 0 && id < (int)elements.size()) {
                elements[id].history.push_back(line);
                if (elements[id].history.size() > 50) elements[id].history.erase(elements[id].history.begin());
                refresh();
            }
        }

        void clear(int id) {
            if (id >= 0 && id < (int)elements.size()) {
                elements[id].history.clear();
                refresh();
            }
        }

        void draw(HDC hdc) {
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_REGULAR, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Segoe UI");
            HFONT hTermFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Consolas");
            
            for (auto& e : elements) {
                RECT r = { e.x, e.y, e.x + e.w, e.y + e.h };
                SelectObject(hdc, (e.type == Type::TERMINAL) ? hTermFont : hFont);

                if (e.type == Type::BUTTON || e.type == Type::INPUT) {
                    COLORREF baseCol = (e.type == Type::INPUT) ? RGB(45, 45, 48) : e.color;
                    if (e.isHovered && e.type == Type::BUTTON) {
                        baseCol = RGB(
                            (std::min)(255, (int)GetRValue(baseCol) + 30),
                            (std::min)(255, (int)GetGValue(baseCol) + 30),
                            (std::min)(255, (int)GetBValue(baseCol) + 30)
                        );
                    }
                    
                    HBRUSH br = CreateSolidBrush(baseCol);
                    HPEN pn = CreatePen(PS_SOLID, 1, e.isFocused ? RGB(0, 120, 215) : RGB(80, 80, 80));
                    SelectObject(hdc, br); SelectObject(hdc, pn);
                    RoundRect(hdc, r.left, r.top, r.right, r.bottom, 5, 5);
                    
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkMode(hdc, TRANSPARENT);
                    std::string disp = e.text + (e.isFocused && e.type == Type::INPUT ? "_" : "");
                    DrawTextA(hdc, disp.c_str(), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    
                    DeleteObject(br); DeleteObject(pn);
                }
                else if (e.type == Type::LABEL) {
                    SetTextColor(hdc, e.color);
                    SetBkMode(hdc, TRANSPARENT);
                    DrawTextA(hdc, e.text.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                }
                else if (e.type == Type::SLIDER) {
                    HPEN pn = CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
                    SelectObject(hdc, pn);
                    MoveToEx(hdc, e.x, e.y + e.h/2, NULL);
                    LineTo(hdc, e.x + e.w, e.y + e.h/2);
                    HBRUSH br = CreateSolidBrush(e.color);
                    int pos = e.x + (int)(e.sliderVal * e.w);
                    RECT knob = { pos - 5, e.y, pos + 5, e.y + e.h };
                    FillRect(hdc, &knob, br);
                    DeleteObject(br); DeleteObject(pn);
                }
                else if (e.type == Type::TERMINAL) {
                    HBRUSH bg = CreateSolidBrush(RGB(20, 20, 20));
                    FillRect(hdc, &r, bg);
                    DeleteObject(bg);
                    SetTextColor(hdc, RGB(0, 255, 0));
                    int yOff = 5;
                    for (auto& line : e.history) {
                        TextOutA(hdc, e.x + 5, e.y + yOff, line.c_str(), (int)line.length());
                        yOff += 16;
                    }
                    std::string inp = "> " + e.text + (e.isFocused ? "_" : "");
                    TextOutA(hdc, e.x + 5, e.y + e.h - 20, inp.c_str(), (int)inp.length());
                }
            }
            DeleteObject(hFont); DeleteObject(hTermFont);
        }

        void onMouseMove(int mx, int my) {
            bool needRefresh = false;
            for (auto& e : elements) {
                bool hover = (mx >= e.x && mx <= e.x + e.w && my >= e.y && my <= e.y + e.h);
                if (e.isHovered != hover) { e.isHovered = hover; needRefresh = true; }
                
                if (e.isDragging && e.type == Type::SLIDER) {
                    float newVal = (float)(mx - e.x) / (float)e.w;
                    e.sliderVal = (std::max)(0.0f, (std::min)(1.0f, newVal));
                    if (e.onSlide) e.onSlide(e.sliderVal);
                    needRefresh = true;
                }
            }
            if (needRefresh) refresh();
        }

        void onMouseDown(int mx, int my) {
            focusedIdx = -1;
            for (int i = 0; i < (int)elements.size(); ++i) {
                auto& e = elements[i];
                e.isFocused = (mx >= e.x && mx <= e.x + e.w && my >= e.y && my <= e.y + e.h);
                if (e.isFocused) {
                    focusedIdx = i;
                    if (e.type == Type::BUTTON && e.onClick) e.onClick();
                    if (e.type == Type::SLIDER) { e.isDragging = true; onMouseMove(mx, my); }
                }
            }
            refresh();
        }

        void onMouseUp() {
            for (auto& e : elements) e.isDragging = false;
        }

        void onChar(char c) {
            if (focusedIdx != -1) {
                auto& e = elements[focusedIdx];
                if (e.type == Type::INPUT || e.type == Type::TERMINAL) {
                    if (c == 8) { if (!e.text.empty()) e.text.pop_back(); }
                    else if (c == 13) {
                        if (e.type == Type::TERMINAL && e.onCommand) {
                            e.history.push_back("> " + e.text);
                            e.onCommand(e.text);
                            e.text = "";
                        }
                    }
                    else if (c >= 32) e.text += c;
                    refresh();
                }
            }
        }

        void refresh() { InvalidateRect(hwnd, NULL, FALSE); }
        void setHwnd(HWND h) { hwnd = h; }
    };

    inline int button(std::string t, int x, int y, int w, int h, COLORREF c, std::function<void()> cb) {
        return (int)Engine::getInstance().add({Type::BUTTON, t, {}, x, y, w, h, c, cb});
    }
    inline int input(int x, int y, int w, int h, std::string t = "") {
        return (int)Engine::getInstance().add({Type::INPUT, t, {}, x, y, w, h, 0});
    }
    inline int label(std::string t, int x, int y, COLORREF c = RGB(255,255,255)) {
        return (int)Engine::getInstance().add({Type::LABEL, t, {}, x, y, 200, 20, c});
    }
    inline int slider(int x, int y, int w, COLORREF c, std::function<void(float)> cb) {
        return (int)Engine::getInstance().add({Type::SLIDER, "", {}, x, y, w, 15, c, nullptr, nullptr, cb});
    }
    inline int terminal(int x, int y, int w, int h, std::function<void(std::string)> cb) {
        return (int)Engine::getInstance().add({Type::TERMINAL, "", {}, x, y, w, h, 0, nullptr, cb});
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_CHAR: Engine::getInstance().onChar((char)wp); break;
        case WM_LBUTTONDOWN: Engine::getInstance().onMouseDown(LOWORD(lp), HIWORD(lp)); break;
        case WM_LBUTTONUP: Engine::getInstance().onMouseUp(); break;
        case WM_MOUSEMOVE: Engine::getInstance().onMouseMove(LOWORD(lp), HIWORD(lp)); break;
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            HDC mdc = CreateCompatibleDC(hdc);
            HBITMAP mbmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mdc, mbmp);
            HBRUSH hbg = CreateSolidBrush(RGB(30, 30, 35));
            FillRect(mdc, &rc, hbg);
            DeleteObject(hbg);
            Engine::getInstance().draw(mdc);
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mdc, 0, 0, SRCCOPY);
            DeleteObject(mbmp); DeleteDC(mdc); EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    inline void init(int w, int h, const char* title) {
        HINSTANCE hi = GetModuleHandle(NULL);
        WNDCLASS wc = {0}; wc.lpfnWndProc = WindowProc; wc.hInstance = hi; wc.lpszClassName = "UIED_WIN";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
        HWND hwnd = CreateWindowEx(0, "UIED_WIN", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, w, h, NULL, NULL, hi, NULL);
        Engine::getInstance().setHwnd(hwnd);
        MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
}
#endif

// 2025 UIED