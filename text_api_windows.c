#include "text_api_windows.h"

#define MAX_NUM_FILES 1024
#define SLOT_IN_USE_FLAG 0

typedef struct {
    char     *data;
    HANDLE   hMapping;
    HANDLE   hFile;
    uint32_t flags;
} file_struct_t;

file_struct_t files[MAX_NUM_FILES];

uint32_t get_last_write_time(const char* filename) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    
    // GetAttributesEx does NOT require opening/locking the file
    if (GetFileAttributesExA(filename, GetFileExInfoStandard, &data)) {
        SYSTEMTIME st;
        // Convert the FILETIME structure to a readable System Time
        FileTimeToSystemTime(&data.ftLastWriteTime, &st);
        
        printf("Last modified: %02d/%02d/%d %02d:%02d:%02d\n",
               st.wDay, st.wMonth, st.wYear, 
               st.wHour, st.wMinute, st.wSecond);
    } else {
        printf("Error: %lu\n", GetLastError());
    }
}

uint64_t get_file_timestamp(const char* filename) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    // Get metadata without opening the file
    if (GetFileAttributesExA(filename, GetFileExInfoStandard, &data)) {
        // Combine the two 32-bit parts into one 64-bit integer
        ULARGE_INTEGER ull;
        ull.LowPart = data.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = data.ftLastWriteTime.dwHighDateTime;

        return ull.QuadPart;
    }

    return 0; // File not found or access denied
}

BOOL file_exists(LPCTSTR f_path)
{
    DWORD dwAttrib = GetFileAttributes(f_path);

    // Check if the call failed AND the specific error was "file not found"
    if (dwAttrib == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND) {
        return FALSE; // File does not exist
    }
    
    // Additional check to ensure it's not a directory if needed
    // return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));

    return TRUE; // File exists (or is a directory/some other object)
}

// Helper function to ensure all directories in a path exist
void ensure_path_exists(const char* path) {
    char temp[MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '/') temp[len - 1] = 0;
    if (temp[len - 1] == '\\') temp[len - 1] = 0;

    // Iterate through the path and create directories one by one
    for (p = temp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char backup = *p;
            *p = 0;
            _mkdir(temp); // Create directory (ignores if already exists)
            *p = backup;
        }
    }
}

HANDLE open_file_no_matter_what(const char* filename) {
    // Usage example: 
    // HANDLE file =open_file_no_matter_what("temp/var/test/test.txt");
    // if (file != INVALID_HANDLE_VALUE) {
    //     // Use the file...
    //     CloseHandle(file);
    // }

    // 1. Ensure the directory structure exists
    ensure_path_exists(filename);

    // 2. Open or create the file
    // GENERIC_READ | GENERIC_WRITE: Allows reading and writing
    // FILE_SHARE_READ: Allows other processes to read while open
    // OPEN_ALWAYS: Opens if exists, creates if not
    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Error opening file: %lu\n", GetLastError());
    } else {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            printf("File opened successfully.\n");
        } else {
            printf("File created successfully.\n");
        }
    }

    return hFile;
}

void process_large_file(const char* filename) {
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) { CloseHandle(hFile); return; }

    // Map the entire file into memory
    const char* fileData = (const char*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (fileData == NULL) { /* Handle error */ }

    // Now 'fileData' behaves like a giant char array.
    // To "get" a line, you just use a pointer to an offset in fileData.
    
    // Cleanup (when done)
    UnmapViewOfFile(fileData);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

typedef struct {
    HANDLE hFile;
    HANDLE hMapping;
    const char* data;    // Pointer to the memory-mapped bytes
    size_t fileSize;
    size_t* lineOffsets; // Array of byte-offsets where lines start
    size_t lineCount;
} MappedFile;

MappedFile* open_and_index_file(const char* filename) {
    MappedFile* mf = (MappedFile*)calloc(1, sizeof(MappedFile));

    // 1. Open with FILE_SHARE_WRITE so oher users can keep working
    mf->hFile = CreateFileA(filename, GENERIC_READ, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (mf->hFile == INVALID_HANDLE_VALUE) {
        free(mf);
        return NULL;
    }

    // 2. Get the current size
    LARGE_INTEGER size;
    GetFileSizeEx(mf->hFile, &size);
    mf->fileSize = (size_t)size.QuadPart;

    if (mf->fileSize == 0) return mf; // Empty file is valid, but nothing to map

    // 3. Create Mapping and View
    mf->hMapping = CreateFileMapping(mf->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    mf->data = (const char*)MapViewOfFile(mf->hMapping, FILE_MAP_READ, 0, 0, 0);

    // 4. Build the initial index
    rebuild_index(mf);

    return mf;
}

void rebuild_index(MappedFile* mf) {
    // 1. Refresh mapping if the file size changed
    LARGE_INTEGER currentSize;
    GetFileSizeEx(mf->hFile, &currentSize);
    
    if ((size_t)currentSize.QuadPart != mf->fileSize) {
        // Unmap old view and mapping object to "stretch" to new size
        if (mf->data) UnmapViewOfFile(mf->data);
        if (mf->hMapping) CloseHandle(mf->hMapping);

        mf->fileSize = (size_t)currentSize.QuadPart;
        mf->hMapping = CreateFileMapping(mf->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        mf->data = (const char*)MapViewOfFile(mf->hMapping, FILE_MAP_READ, 0, 0, 0);
    }

    // 2. Clear old index
    if (mf->lineOffsets) free(mf->lineOffsets);

    // 3. Scan for newlines
    size_t capacity = (mf->fileSize / 50) + 10; // Heuristic: ~50 chars per line
    mf->lineOffsets = (size_t*)malloc(capacity * sizeof(size_t));
    mf->lineCount = 0;

    if (mf->fileSize > 0) {
        mf->lineOffsets[mf->lineCount++] = 0; // First line always starts at 0
        for (size_t i = 0; i < mf->fileSize; i++) {
            if (mf->data[i] == '\n') {
                if (mf->lineCount >= capacity) {
                    capacity *= 2;
                    mf->lineOffsets = (size_t*)realloc(mf->lineOffsets, capacity * sizeof(size_t));
                }
                // If there's a character after the \n, that's a new line
                if (i + 1 < mf->fileSize) {
                    mf->lineOffsets[mf->lineCount++] = i + 1;
                }
            }
        }
    }
}

// Usage example for the mmapped file:
void print_line(MappedFile* mf, size_t lineIdx) {
    if (lineIdx >= mf->lineCount) return;

    size_t start = mf->lineOffsets[lineIdx];
    size_t end = (lineIdx + 1 < mf->lineCount) ? mf->lineOffsets[lineIdx + 1] : mf->fileSize;

    // Use fwrite to print the specific slice of memory
    fwrite(mf->data + start, 1, end - start, stdout);
}

void close_mapped_file(MappedFile* mf) {
    if (!mf) return;
    if (mf->lineOffsets) free(mf->lineOffsets);
    if (mf->data) UnmapViewOfFile(mf->data);
    if (mf->hMapping) CloseHandle(mf->hMapping);
    if (mf->hFile) CloseHandle(mf->hFile);
    free(mf);
}


int find_free_idx(void) {
    for(int i=0; i<MAX_NUM_FILES; i++) {
        if (files[i].flags & (1 << SLOT_IN_USE_FLAG) == 0) {
            return i;
        }
    }
    return -1;
}

uint32_t open_file(const char *filename) {
    
    int idx = find_free_idx();
    if (idx == -1) {    // Check if we have free slot
        return -1;
    }

    // 1. Open the file
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Could not open file (Error %lu)\n", GetLastError());
        return 1;
    }

    // 2. Get file size (needed for mapping and bounds checking)
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0) {
        CloseHandle(hFile);
        return 0; 
    }

    // 3. Create a file mapping object
    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        printf("Could not create file mapping (Error %lu)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // 4. Map the view of the file into memory
    char* pData = (char*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (pData == NULL) {
        printf("Could not map view of file (Error %lu)\n", GetLastError());
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 1;
    }
}

void close_file(uint32_t file_idx) {
    if (file_idx < MAX_NUM_FILES) {
        UnmapViewOfFile(files[file_idx].data);
        CloseHandle(files[file_idx].hMapping);
        CloseHandle(files[file_idx].hFile);
        files[file_idx].flags ^= (uint32_t)1 << SLOT_IN_USE_FLAG;
    }
}
