// Build: gcc memory_mapping_example.c -mwindows -mconsole -o mmap.exe
#include <windows.h>
#include <stdio.h>

int main() {
    const char* filename = "text_api_windows.c"; // Hardcoded filename

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

    // 5. Process the file: Count lines and store pointers
    // For simplicity, we assume a max of 10,000 lines
    char* linePointers[10000];
    int lineCount = 0;

    if (fileSize > 0) {
        char* currentPos = pData;
        char* endOfData = pData + fileSize;

        while (currentPos < endOfData && lineCount < 10000) {
            linePointers[lineCount++] = currentPos;

            // Search for the next newline character efficiently
            char* nextNewline = (char*)memchr(currentPos, '\n', endOfData - currentPos);

            if (nextNewline) {
                currentPos = nextNewline + 1; // Move to the start of the next line
            } else {
                break; // No more newlines found
            }
        }
    }

    printf("Total lines found: %d\n", lineCount);
    if (lineCount > 0) {
        printf("First line starts with: %.10s...\n", linePointers[0]);
    }

    // 6. Cleanup
    UnmapViewOfFile(pData);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    return 0;
}