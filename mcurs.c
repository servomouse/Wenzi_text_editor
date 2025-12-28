#include <windows.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_CURSORS 5
#define BUFFER_SIZE 1024

typedef struct {
    int position; // Position of the cursor in the text buffer
} Cursor;

char textBuffer[BUFFER_SIZE]; // Simple text buffer
Cursor cursors[MAX_CURSORS]; // Array to store cursor positions
int cursorCount = 1; // Start with one cursor at position 0
int cursorVisible = 1; // 1 for visible, 0 for hidden
#define ID_CURSOR_TIMER 1

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawTextWithCursors(HDC hdc);
void InsertTextAtCursor(char character);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "CustomTextEditor";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Draw text and cursors
            DrawTextWithCursors(hdc);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            if (wParam == ID_CURSOR_TIMER) {
                cursorVisible = !cursorVisible; // Toggle visibility
                InvalidateRect(hwnd, NULL, TRUE); // Trigger a repaint
            }
            return 0;
        }

        case WM_CHAR: {
            if (wParam >= 32 && wParam <= 126) { // ASCII range for printable characters
                cursorVisible = 1; // Force cursor to show immediately when typing
                char character = (char)wParam;
                InsertTextAtCursor(character);
                InvalidateRect(hwnd, NULL, TRUE);
                // Optional: Reset timer here so it doesn't blink out mid-type
                SetTimer(hwnd, ID_CURSOR_TIMER, 500, NULL);
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            // For simplicity, set cursor to the end of the text
            if (cursorCount < MAX_CURSORS) {
                cursors[cursorCount++].position = strlen(textBuffer); // Simple click handler
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
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
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    int startX = 10;
    int startY = 10;
    
    // Draw the current text
    TextOut(hdc, startX, startY, textBuffer, (int)strlen(textBuffer));

    if (!cursorVisible) {
        return;
    }

    // Draw cursors
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
