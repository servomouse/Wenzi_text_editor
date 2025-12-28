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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawTextWithCursors(HDC hdc);
void InsertTextAtCursor(char character);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "CustomTextEditor";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

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

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Initialize text buffer
    strcpy(textBuffer, "Edit your text here...");

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

        case WM_KEYDOWN: {
            if (wParam >= 32 && wParam <= 126) { // ASCII range for printable characters
                char character = (char)wParam;
                InsertTextAtCursor(character);
                InvalidateRect(hwnd, NULL, TRUE);
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
    // Draw the current text
    TextOut(hdc, 10, 10, textBuffer, strlen(textBuffer));

    // Draw cursors
    for (int i = 0; i < cursorCount; i++) {
        int cursorX = 10 + cursors[i].position * 10; // Placeholder for actual cursor position
        MoveToEx(hdc, cursorX, 10, NULL);
        LineTo(hdc, cursorX, 20); // Drawing cursor as a vertical line
    }
}
