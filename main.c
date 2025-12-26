#include <windows.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define DRAWING_AREA_WIDTH 100
#define MAX_ARGS 10
#define ARG_LENGTH 100

void CreateConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONIN$", "r", stdin);
}

// Global variable for the text field
HWND hTextField;
WNDPROC editProc; // Store the old procedure
HWND hDrawingArea;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DrawingAreaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

    const char CLASS_NAME[] = "Sample Window Class";
    const char DRAWING_CLASS_NAME[] = "Drawing Area Class";

    // Register main window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    
    // Register drawing area class
    WNDCLASS wcDrawing = {0};
    wcDrawing.lpfnWndProc = DrawingAreaProc;
    wcDrawing.hInstance = hInstance;
    wcDrawing.lpszClassName = DRAWING_CLASS_NAME;
    RegisterClass(&wcDrawing);

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

    // Create the drawing area as a separate window
    hDrawingArea = CreateWindowEx(
        0,
        DRAWING_CLASS_NAME,
        "Drawing Area",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        WINDOW_WIDTH - DRAWING_AREA_WIDTH, 0, DRAWING_AREA_WIDTH, WINDOW_HEIGHT,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Set default text in the text field
    SetWindowText(hTextField, "Welcome! Type your text here...");

    // Subclass the EDIT control
    editProc = (WNDPROC)SetWindowLongPtr(hTextField, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

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
    return CallWindowProc(editProc, hwnd, uMsg, wParam, lParam);
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
                MoveWindow(hTextField, 0, 0, rect.right - DRAWING_AREA_WIDTH, rect.bottom, TRUE);
            // Update the sidebar
            if (hDrawingArea) {
                MoveWindow(hDrawingArea, rect.right - DRAWING_AREA_WIDTH, 0, DRAWING_AREA_WIDTH, rect.bottom, TRUE);
            }
        }
    }

    // Call the default handler for other messages
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DrawingAreaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Set the background color of the drawing area
            HBRUSH brush = CreateSolidBrush(RGB(240, 240, 240)); // Light gray background
            FillRect(hdc, &ps.rcPaint, brush);
            DeleteObject(brush);

            MoveToEx(hdc, 10, 10, NULL);
            LineTo(hdc, 90, 10);    // Draw a line

            Rectangle(hdc, 10, 30, 90, 110);    // Draw a rectangle

            Ellipse(hdc, 10, 120, 90, 200); // Draw an ellipse

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    // Call default procedure for unhandled messages
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
