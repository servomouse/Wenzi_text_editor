#include <windows.h>
#include <stdio.h>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400

void CreateConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
}

// Global variable for the text field
HWND hTextField;
WNDPROC oldEditProc; // Store the old procedure

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CreateConsole();

    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Multi-line Text Field Application",
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

    // Create a text field (EDIT control) that takes the full window size
    hTextField = CreateWindowEx(
        0,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN,
        0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Set default text in the text field
    SetWindowText(hTextField, "Welcome! Type your text here...");

    // Subclass the EDIT control
    oldEditProc = (WNDPROC)SetWindowLongPtr(hTextField, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        // Check if Ctrl is pressed
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
            // Detect Ctrl + C
            if (wParam == 'C') {
                printf("Ctrl+C detected\n");
                
                return 0; // Prevent default handling
            }
        }
        // Print the virtual key code to the console
        printf("Key pressed in text field: %d\n", wParam);
    } else if (uMsg == WM_LBUTTONDOWN) {
        // Handle left mouse button clicks
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        printf("Left mouse button clicked at (%d, %d)\n", pt.x, pt.y);
    } else if (uMsg == WM_LBUTTONUP) {
        // Handle left mouse button clicks
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        printf("Left mouse button released at (%d, %d)\n", pt.x, pt.y);
    } else if (uMsg == WM_RBUTTONDOWN) {
        // Handle right mouse button clicks
        DWORD selStart, selEnd;
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

        if (selStart != selEnd) { // There is a selection
            // Get the starting line and column of the selection
            int startLine = SendMessage(hwnd, EM_LINEFROMCHAR, (WPARAM)selStart, 0);
            int startCol = selStart - SendMessage(hwnd, EM_LINEINDEX, (WPARAM)startLine, 0);

            // Get the ending line and column of the selection
            int endLine = SendMessage(hwnd, EM_LINEFROMCHAR, (WPARAM)selEnd, 0);
            int endCol = selEnd - SendMessage(hwnd, EM_LINEINDEX, (WPARAM)endLine, 0);

            printf("Text selected from Line %d, Col %d to Line %d, Col %d\n", 
                   startLine + 1, startCol + 1, endLine + 1, endCol + 1);
        } else {
            printf("No text selected.\n");
        }
    }

    // Call the original procedure for default processing
    return CallWindowProc(oldEditProc, hwnd, uMsg, wParam, lParam);
}

void DrawShapes(HDC hdc) {
    // Set drawing color
    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));

    // Draw a line
    MoveToEx(hdc, 10, 10, NULL);
    LineTo(hdc, 300, 10);

    // Draw a rectangle
    Rectangle(hdc, 50, 20, 350, 100);

    // Draw an ellipse
    Ellipse(hdc, 100, 120, 300, 220);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    // Handle resizing the text field when the window is resized
    if (uMsg == WM_SIZE) {
        if (hTextField) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            MoveWindow(hTextField, 0, 0, rect.right, rect.bottom, TRUE);
        }
    }
    if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawShapes(hdc);
        EndPaint(hwnd, &ps);
    }

    // Call the default handler for other messages
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
