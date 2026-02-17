// Build: gcc term.c -o app.exe -mwindows -mconsole
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
    int is_initialized;
    int win_width_px;
    int win_height_px;
    int win_width_char;
    int win_height_char;
    int char_width_px;
    int char_height_px;
} window_layout_t;

window_layout_t window_layout;

typedef struct {
    char **lines;
    uint32_t num_lines;
    uint32_t top_line;
    int selected_line;  // -1 for no selection
} fs_buffer_t;

typedef struct {
    fs_buffer_t *bufs;
    uint32_t num_buffers;
    int current_buffer_idx;
} fs_walker_t;

fs_walker_t fs_walker = {
    .bufs = NULL,
    .num_buffers = 0,
    .current_buffer_idx = -1
};

uint32_t random_int(uint32_t min, uint32_t max) {
    return (rand() % (max - min + 1)) + min;
}

void generate_random_string(char *str, int num_elements) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,-_ ";
    int charset_size = sizeof(charset) - 1;

    for (int i = 0; i < num_elements; i++) {
        int key = rand() % charset_size;
        str[i] = charset[key];
    }
    str[num_elements] = '\0';
}

uint32_t fs_walker_add_buffer(void) {
    // Creates new fs_buffer and returns its idx
    uint32_t new_buf_idx = fs_walker.num_buffers;
    if (fs_walker.current_buffer_idx == -1) {
        fs_walker.bufs = calloc(1, sizeof(fs_buffer_t));
        fs_walker.current_buffer_idx = 0;
    } else {
        fs_walker.bufs = realloc(fs_walker.bufs, sizeof(fs_buffer_t) * fs_walker.num_buffers+1);
    }
    fs_walker.num_buffers++;
    fs_walker.bufs[new_buf_idx].lines = NULL;
    fs_walker.bufs[new_buf_idx].num_lines = 0;
    fs_walker.bufs[new_buf_idx].top_line = 0;
    fs_walker.bufs[new_buf_idx].selected_line = -1;
    return new_buf_idx;
}

void fs_walker_update_buf(uint32_t buf_idx) {
    if(fs_walker.bufs[buf_idx].lines) {
        for(size_t i=0; i<fs_walker.bufs[buf_idx].num_lines; i++) {
            free(fs_walker.bufs[buf_idx].lines[i]);
        }
        free(fs_walker.bufs[buf_idx].lines);
    }
    uint32_t num_lines = random_int(0, 64);
    fs_walker.bufs[buf_idx].num_lines = num_lines;
    fs_walker.bufs[buf_idx].selected_line = -1;
    fs_walker.bufs[buf_idx].lines = calloc(num_lines, sizeof(char*));
    for(size_t i=0; i<num_lines; i++) {
        fs_walker.bufs[buf_idx].lines[i] = calloc(MAX_PATH, sizeof(char));
        generate_random_string(fs_walker.bufs[buf_idx].lines[i], random_int(10, 100));
        // printf("Generated line: %s\n", fs_walker.bufs[buf_idx].lines[i]);
    }
}

void fs_walker_switch_to_buf(uint32_t buf_idx) {
    if (buf_idx < fs_walker.num_buffers) {
        printf("Switching to buffer %d\n", buf_idx);
        fs_walker.current_buffer_idx = buf_idx;
        fs_walker_update_buf(buf_idx);
    } else {
        fprintf(stderr, "Error: Trying to switch to a non-existent buffer %d (number of buffers: %d)\n", buf_idx, fs_walker.num_buffers);
    }
}

fs_buffer_t * fs_walker_get_current_buf(void) {
    if (fs_walker.num_buffers > 0)
        return &fs_walker.bufs[fs_walker.current_buffer_idx];
    else
        return NULL;
}

void fs_walker_scroll(int steps) {
    // Positive scrolls down
    fs_buffer_t *cb = fs_walker_get_current_buf();
    int temp_pos = cb->top_line + steps;
    if (temp_pos < 0)
        temp_pos = 0;
    else if (temp_pos >= cb->num_lines)
        temp_pos = cb->num_lines - 1;
    cb->top_line = (uint32_t)temp_pos;
}

void fs_walker_move_selected_line(int steps) {
    fs_buffer_t *cb = fs_walker_get_current_buf();
    if (steps == INT_MAX) {
        cb->selected_line = cb->num_lines-1;
        fs_walker_scroll(cb->num_lines - (window_layout.win_height_char - 1));
    } else if (steps == INT_MIN) {
        cb->selected_line = 0;
        cb->top_line = 0;
    } else {
        int temp_pos = cb->selected_line + steps;
        if (temp_pos < 0) temp_pos = 0;
        else if (temp_pos >= cb->num_lines) temp_pos = cb->num_lines-1;
        cb->selected_line = temp_pos;
    }
    if (cb->selected_line < cb->top_line) {
        cb->top_line = cb->selected_line;
    } else if (cb->selected_line >= (cb->top_line + window_layout.win_height_char - 1)) {
        int delta = cb->selected_line - (window_layout.win_height_char - 2) - cb->top_line;
        fs_walker_scroll(delta);
        printf("[INFO] Selected line is too low: selected_line: %d; top_line: %d; win_height: %d; delta: %d\n",
            cb->selected_line,
            cb->top_line,
            window_layout.win_height_char,
            delta);
    }
}

char textBuffer[BUFFER_SIZE]; // Simple text buffer
Cursor cursors[MAX_CURSORS]; // Array to store cursor positions
int cursorCount = 1; // Start with one cursor at position 0
int cursorVisible = 1; // 1 for visible, 0 for hidden
HFONT hEditorFont = NULL;
int fontSize = 20; // Default height in pixels
int cursor_active = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void draw_text(HDC hdc);
// void InsertTextAtCursor(char character);
void update_editor_layout(HWND hwnd);
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
    // CreateConsole();
    freopen("CONOUT$", "w", stderr);    // Make stderr work
    setvbuf(stderr, NULL, _IONBF, 0);   // Disable stderr cache
    // setvbuf(stdout, NULL, _IONBF, 0);   // Disable stdout cache
    fprintf(stderr, "STDERR output test\n");
    
    handle_arguments(lpCmdLine);
    srand(42);

    fs_walker_add_buffer();
    fs_walker_switch_to_buf(0);


    const char CLASS_NAME[] = "CustomTextEditor";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = NULL;  // Don't set a default cursor; will handle it manually

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
        return -1;
    }
    update_editor_font();
    // SetTimer(hwnd, ID_CURSOR_TIMER, 500, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Initialize text buffer
    strcpy(textBuffer, "Edit your text here...");
    // cursors[0].position = 5;    // Set first cursor to the end
    // cursorCount = 1;

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
    printf("wm_destroy_cb()\n");
    if (hEditorFont)
        DeleteObject(hEditorFont);
    PostQuitMessage(0);
    return 0;
}

LRESULT wm_paint_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Get dimensions
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Create the "Invisible Canvas" (Back Buffer)
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, memBitmap);

    // Clear the background on the memDC
    HBRUSH hBackground = (HBRUSH)(COLOR_WINDOW + 1);
    FillRect(memDC, &rect, hBackground);

    // Draw the Text on the memDC
    draw_text(memDC);

    // "Flip" the buffer: Copy from memory to the screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // Cleanup
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    EndPaint(hwnd, &ps);
    update_editor_layout(hwnd);
    return 0;
}

LRESULT wm_timer_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (wParam == ID_CURSOR_TIMER) {
        if (cursor_active) {
            cursorVisible = !cursorVisible; // Toggle visibility
            InvalidateRect(hwnd, NULL, FALSE); // Trigger a repaint
        }
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
        // char character = (char)wParam;
        // InsertTextAtCursor(character);
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
    // POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    InvalidateRect(hwnd, NULL, FALSE);
    printf("Cursor coords: (x = %d, y = %d) (xc = %d, yc = %d), cw = %d, ch = %d\n",
            xPos, yPos,
            xPos / window_layout.char_width_px, yPos / window_layout.char_height_px,
            window_layout.char_width_px, window_layout.char_height_px);
    return 1;
}

LRESULT wm_mousewheel_cb(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    int wheel_step = 120;
    int16_t wheel_rot = (int16_t)HIWORD(wParam) / wheel_step;
    // int virt_keys = LOWORD(wParam);
    // printf("Wheel scrolling: %d, virtual keys: 0x%X)\n", wheel_rot, virt_keys);
    fs_walker_scroll(-1 * wheel_rot);
    InvalidateRect(hwnd, NULL, FALSE);
    return 1;
}

LRESULT wm_control_key_cb(uint32_t is_key_down, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // printf("wParam value: 0x%llXd\n", wParam);
    int need_redraw = 0;
    switch(wParam) {
        case VK_UP:     // Arrow Up
            if(is_key_down) {
                fs_walker_move_selected_line(-1);
                need_redraw = 1;
                // printf("Key pressed: Arrow Up\n");
            } else
                printf("Key released: Arrow Up\n");
            break;
        case VK_RIGHT:     // Arrow Right
            if(is_key_down)
                printf("Key pressed: Arrow Right\n");
            else
                printf("Key released: Arrow Right\n");
            break;
        case VK_DOWN:     // Arrow Down
            if(is_key_down){
                fs_walker_move_selected_line(1);
                need_redraw = 1;
                // printf("Key pressed: Arrow Down\n");
            } else
                printf("Key released: Arrow Down\n");
            break;
        case VK_LEFT:     // Arrow Left
            if(is_key_down)
                printf("Key pressed: Arrow Left\n");
            else
                printf("Key released: Arrow Left\n");
            break;
        default:
            printf("Unknown key: 0x%llXd\n", wParam);
    }
    if (need_redraw) {
        InvalidateRect(hwnd, NULL, FALSE);
    }
    return 1;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:        return wm_destroy_cb(hwnd, uMsg, wParam, lParam);
        case WM_PAINT:          return wm_paint_cb(hwnd, uMsg, wParam, lParam);
        case WM_TIMER:          return wm_timer_cb(hwnd, uMsg, wParam, lParam);
        case WM_MOUSEWHEEL:     return wm_mousewheel_cb(hwnd, uMsg, wParam, lParam);
        case WM_ERASEBKGND:     return wm_erasebkgnd_cb(hwnd, uMsg, wParam, lParam);
        case WM_LBUTTONDOWN:    return wm_lbuttondown_cb(hwnd, uMsg, wParam, lParam);
        case WM_KEYDOWN:        return wm_control_key_cb(1, hwnd, uMsg, wParam, lParam);
        case WM_KEYUP:          return wm_control_key_cb(0, hwnd, uMsg, wParam, lParam);
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
        // default: // There are quite a lot of unknown uMsgs, ignore them
        //     printf("Error: Unknown uMsg: 0x%Xd\n", uMsg);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void draw_text(HDC hdc) {
    if (!window_layout.is_initialized) return;

    // Select our custom font and save the old one (standard GDI rule)
    HFONT hOldFont = (HFONT)SelectObject(hdc, hEditorFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    int text_height = tm.tmHeight;
    int text_top_offset = window_layout.char_width_px;
    
    int win_height = (window_layout.win_height_px - text_top_offset)/ window_layout.char_height_px;
    int win_width = window_layout.win_width_char - 1;
    // printf("Text height: %d px, window height: %d lines, window width: %d lines\n", text_height, win_height, win_width);
    
    int text_left_offset = window_layout.char_width_px;
    // for(int i=0; i<win_height; i++) {
    //     TextOut(hdc, text_left_offset, text_top_offset+(text_height*i), textBuffer, (int)strlen(textBuffer));
    // }

    fs_buffer_t * buf = fs_walker_get_current_buf();
    if (buf) {
        uint32_t num_lines_to_display = buf->num_lines - buf->top_line;
        if (num_lines_to_display > win_height) {
            num_lines_to_display = win_height;
        }
        // printf("Num lines to display: %d\n", num_lines_to_display);
        for(int i=0; i<num_lines_to_display; i++) {
            size_t idx = buf->top_line+i;
            uint32_t is_selected_line = idx == buf->selected_line? 1: 0;
            char *line = buf->lines[idx];
            uint32_t line_len = strlen(line);
            if (line_len > win_width) {
                line_len = win_width;
            }

            COLORREF previousColor;
            COLORREF previousBkColor;
            int previousBkMode;
            if (is_selected_line) {
                previousColor = SetTextColor(hdc, RGB(255, 0, 0));
                previousBkColor = SetBkColor(hdc, RGB(255, 255, 0));
                // Ensure the background mode is OPAQUE (the default, but good practice to ensure)
                previousBkMode = SetBkMode(hdc, OPAQUE);
            }
            TextOut(hdc, text_left_offset, text_top_offset+(text_height*i), line, (int)line_len);
            if (is_selected_line) {
                SetTextColor(hdc, previousColor);
                SetBkColor(hdc, previousBkColor);
                SetBkMode(hdc, previousBkMode);
            }
        }
    }
        
    SelectObject(hdc, hOldFont);
}

void update_editor_layout(HWND hwnd) {
    // printf("Updating editor layout");
    RECT rect;
    GetClientRect(hwnd, &rect);
    window_layout.win_width_px = rect.right - rect.left;
    window_layout.win_height_px = rect.bottom - rect.top;

    HDC hdc = GetDC(hwnd);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hEditorFont);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    
    window_layout.char_width_px = tm.tmAveCharWidth;
    window_layout.char_height_px = tm.tmHeight;
    window_layout.win_width_char = window_layout.win_width_px / tm.tmAveCharWidth;
    window_layout.win_height_char = window_layout.win_height_px / tm.tmHeight;

    window_layout.is_initialized = 1;

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
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
