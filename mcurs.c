#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "core.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_CURSORS 5
#define BUFFER_SIZE 1024
#define ID_CURSOR_TIMER 1
#define MINIMAP_WIDTH 100
#define MAX_ARGS 10
#define ARG_LENGTH 100

// TODO: Add Gutter

typedef struct {
    int position; // Position of the cursor in the text buffer
} Cursor;

typedef struct {
    int win_width_px, win_height_px;
    int win_width_char, win_height_char;
    int char_width, char_height;
} window_layout_t;

char textBuffer[BUFFER_SIZE]; // Simple text buffer
Cursor cursors[MAX_CURSORS]; // Array to store cursor positions
int cursorCount = 1; // Start with one cursor at position 0
int cursorVisible = 1; // 1 for visible, 0 for hidden
HFONT hEditorFont = NULL;
int fontSize = 20; // Default height in pixels

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawTextWithCursors(HDC hdc);
void InsertTextAtCursor(char character);
window_layout_t get_editor_layout(HWND hwnd);
void update_editor_font(void);
void change_font_size(HWND hwnd, int delta);

void CreateConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
}

void handle_arguments(char *cmdline_args) {
    char* arguments[MAX_ARGS];
    int argCount = 0;

    // Tokenize the command line arguments
    char* token = strtok(cmdline_args, " ");
    while (token != NULL && argCount < MAX_ARGS) {
        arguments[argCount++] = token;
        token = strtok(NULL, " ");
    }

    for (int i = 0; i < argCount; i++) {
        printf("Argument %d: %s\n", i + 1, arguments[i]);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // lpCmdLine is all the command line arguments as a single string
    CreateConsole();
    
    handle_arguments(lpCmdLine);
    const char CLASS_NAME[] = "CustomTextEditor";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = NULL;  // Don't try to set a default cursor; I will handle it manually

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Custom Text Editor with Multiple Cursors",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }
    update_editor_font();
    SetTimer(hwnd, ID_CURSOR_TIMER, 500, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Initialize text buffer
    strcpy(textBuffer, "Edit your text here...");
    cursors[0].position = 5;    // strlen(textBuffer); // Set first cursor to the end
    cursorCount = 1;

    // Update window to show the text
    InvalidateRect(hwnd, NULL, TRUE); // Tells Windows the window needs a refresh
    UpdateWindow(hwnd);               // Forces an immediate WM_PAINT message

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT wm_destroy_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (hEditorFont)
        DeleteObject(hEditorFont);
    PostQuitMessage(0);
    return 0;
}

LRESULT wm_paint_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // 1. Get dimensions
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // 2. Create the "Invisible Canvas" (Back Buffer)
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, memBitmap);

    // 3. Clear the background on the memDC
    HBRUSH hBackground = (HBRUSH)(COLOR_WINDOW + 1);
    FillRect(memDC, &rect, hBackground);

    // 4. Draw the Minimap on the memDC
    RECT sidebarRect = { rect.right - 100, 0, rect.right, rect.bottom };
    HBRUSH hSidebarBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(memDC, &sidebarRect, hSidebarBrush);
    DeleteObject(hSidebarBrush);

    // 5. Draw the Text and Cursors on the memDC
    // (Update DrawTextWithCursors to accept the DC it should draw on)
    DrawTextWithCursors(memDC);

    // 6. "Flip" the buffer: Copy from memory to the screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // 7. Cleanup
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
    return 0;
}

LRESULT wm_timer_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (wParam == ID_CURSOR_TIMER) {
        cursorVisible = !cursorVisible; // Toggle visibility
        InvalidateRect(hwnd, NULL, FALSE); // Trigger a repaint
    }
    return 0;
}

LRESULT wm_setcursor_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // loword of lParam tells us where the mouse is hitting (HTCLIENT means inside the window)
    if (LOWORD(lParam) == HTCLIENT) {
        POINT pt;
        GetCursorPos(&pt);           // Get screen coordinates
        ScreenToClient(hwnd, &pt);   // Convert to window coordinates

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Check if mouse is over the Minimap
        if (pt.x >= rect.right - MINIMAP_WIDTH) {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        } else {
            SetCursor(LoadCursor(NULL, IDC_IBEAM));
        }
        return 1; // Tell Windows we handled the cursor
    }
    return 0;
}

LRESULT wm_char_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (wParam >= 32 && wParam <= 126) { // ASCII range for printable characters
        cursorVisible = 1; // Force cursor to show immediately when typing
        char character = (char)wParam;
        InsertTextAtCursor(character);
        InvalidateRect(hwnd, NULL, FALSE);
        // Optional: Reset timer here so it doesn't blink out mid-type
        SetTimer(hwnd, ID_CURSOR_TIMER, 500, NULL);
        return 1;
    }
    return 0;
}

LRESULT wm_erasebkgnd_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return 1; // Tell Windows "I've handled erasing", even though we do nothing.
}

LRESULT wm_lbuttondown_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    // For simplicity, set cursor to the end of the text
    if (cursorCount < MAX_CURSORS) {
        cursors[cursorCount++].position = strlen(textBuffer); // Simple click handler
        InvalidateRect(hwnd, NULL, FALSE);
    }
    printf("Cursor coords: (x = %d, y = %d)\n", xPos, yPos);
    return 1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: {
            return wm_destroy_cb(hwnd, uMsg, wParam, lParam);
        }
        case WM_PAINT: {
            return wm_paint_cb(hwnd, uMsg, wParam, lParam);
        }
        case WM_TIMER: {
            return wm_timer_cb(hwnd, uMsg, wParam, lParam);
        }
        case WM_SETCURSOR: {
            if (wm_setcursor_cb(hwnd, uMsg, wParam, lParam)) {
                return 1;
            }
            break;
        }
        case WM_CHAR: {
            if (wm_char_cb(hwnd, uMsg, wParam, lParam)) {
                return 1;
            }
            break;
        }
        case WM_ERASEBKGND: {
            return wm_erasebkgnd_cb(hwnd, uMsg, wParam, lParam);
        }
        case WM_LBUTTONDOWN: {
            return wm_lbuttondown_cb(hwnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InsertTextAtCursor(char character) {
    for (int i = 0; i < cursorCount; i++) {
        int pos = cursors[i].position;
        if (strlen(textBuffer) < BUFFER_SIZE - 1) { // Check buffer size before inserting
            // Shift text to the right from the cursor position
            memmove(&textBuffer[pos + 1], &textBuffer[pos], strlen(textBuffer) - pos + 1);
            textBuffer[pos] = character; // Insert the character
        }
    }
}

void DrawTextWithCursors(HDC hdc) {
    // Select our custom font and save the old one (standard GDI rule)
    HFONT hOldFont = (HFONT)SelectObject(hdc, hEditorFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    int startX = 10;
    int startY = 10;
    
    // Draw the current text
    TextOut(hdc, startX, startY, textBuffer, (int)strlen(textBuffer));

    if (cursorVisible) {    // Draw cursors
        for (int i = 0; i < cursorCount; i++) {
            SIZE size;
            int pos = cursors[i].position;
            
            // Measure width up to the cursor
            GetTextExtentPoint32(hdc, textBuffer, pos, &size);

            int cursorX = startX + size.cx;
            
            // Use the font metrics for the Y coordinates:
            // Top: startY
            // Bottom: startY + height of the font
            MoveToEx(hdc, cursorX, startY, NULL);
            LineTo(hdc, cursorX, startY + tm.tmHeight); 
        }
    }
    SelectObject(hdc, hOldFont);
}

window_layout_t get_editor_layout(HWND hwnd) {
    window_layout_t layout;
    RECT rect;
    GetClientRect(hwnd, &rect);
    layout.win_width_px = rect.right - rect.left;
    layout.win_height_px = rect.bottom - rect.top;

    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hEditorFont);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    
    layout.char_width = tm.tmAveCharWidth;
    layout.char_height = tm.tmHeight;
    layout.win_width_char = layout.win_width_px / tm.tmAveCharWidth;
    layout.win_height_char = layout.win_height_px / tm.tmHeight;

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
    return layout;
}

void update_editor_font(void) {
    // If a font already exists, delete it to prevent memory leaks
    if (hEditorFont) DeleteObject(hEditorFont);

    hEditorFont = CreateFont(
        fontSize,                        // Height
        0,                               // Width (0 = auto)
        0, 0,                            // Escapement/Orientation
        FW_NORMAL,                       // Weight
        FALSE, FALSE, FALSE,             // Italic, Underline, Strikeout
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,               // High quality rendering
        FIXED_PITCH | FF_MODERN,         // Force Monospaced
        "Consolas"                       // Font Name
    );
}

void change_font_size(HWND hwnd, int delta) {
    fontSize += delta;
    if (fontSize < 8) fontSize = 8;   // Minimum limit
    if (fontSize > 72) fontSize = 72; // Maximum limit
    
    update_editor_font();
    InvalidateRect(hwnd, NULL, FALSE);
}

void show_error(const char* message) {
    MessageBoxA(
        NULL,              // Parent window (NULL = none)
        message,           // The message text
        "System Error",    // The window title
        MB_OK | MB_ICONERROR | MB_SETFOREGROUND
    );
}
