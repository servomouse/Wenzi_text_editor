#include <windows.h>
#include <stdio.h>

/* Git interactions: */

int is_git_repo() {
    FILE* fp = _popen("git rev-parse --is-inside-work-tree 2>nul", "r");
    char result[10];
    if (fp && fgets(result, sizeof(result), fp)) {
        _pclose(fp);
        return (strncmp(result, "true", 4) == 0);
    }
    if (fp) _pclose(fp);
    return 0;
}

void print_git_branch() {
    FILE* fp = _popen("git rev-parse --abbrev-ref HEAD", "r");
    char branch[256];
    if (fp && fgets(branch, sizeof(branch), fp)) {
        printf("Current Branch: %s", branch);
    }
    if (fp) _pclose(fp);
}

void print_file_git_info(const char* filename) {
    char cmd[512];
    // Format: "Author | Date | Message"
    snprintf(cmd, sizeof(cmd), "git log -1 --format=\"%%an | %%ad | %%s\" -- %s", filename);

    FILE* fp = _popen(cmd, "r");
    char buffer[1024];
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        printf("File: %s\nInfo: %s\n", filename, buffer);
    } else {
        printf("File %s has no git history.\n", filename);
    }
    if (fp) _pclose(fp);
}

/* !Git interactions */

// Change Directory	SetCurrentDirectory("C:\\Path\\To\\Folder");
// Get Current Path	GetCurrentDirectory(MAX_PATH, buffer);
// Create Directory	CreateDirectory("NewFolder", NULL);

void print_current_directory() {
    char buffer[MAX_PATH];
    
    // GetCurrentDirectory returns the number of characters written
    // If it fails, it returns 0
    DWORD length = GetCurrentDirectory(MAX_PATH, buffer);

    if (length == 0) {
        printf("Failed to get current directory. Error: %lu\n", GetLastError());
    } else if (length > MAX_PATH) {
        // This handles cases where the path is longer than our buffer
        printf("Buffer too small. Required length: %lu\n", length);
    } else {
        printf("Current Directory: %s\n", buffer);
    }
}

void check_path_type(const char* path) {
    DWORD dwAttrib = GetFileAttributes(path);

    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        printf("Path does not exist.\n");
        return;
    }

    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
        printf("%s is a DIRECTORY\n", path);
    } else {
        printf("%s is a FILE\n", path);
    }
}

void print_file_time(FILETIME ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    printf("%02d/%02d/%d %02d:%02d\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute);
}

void list_directory(const char* path) {
    WIN32_FIND_DATA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[MAX_PATH];

    // Create a search pattern (e.g., "C:\myfolder\*")
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    hFind = FindFirstFile(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Directory not found or empty.\n");
        return;
    }

    do {
        // Skip the "." and ".." directories
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
            printf("%s [%s]\n", findData.cFileName, 
                   (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "DIR" : "FILE");
        }
    } while (FindNextFile(hFind, &findData) != 0);

    FindClose(hFind);
}