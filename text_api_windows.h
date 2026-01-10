#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>
#include <stdint.h>

/** @brief Try to open a file filename
 *  @param filename: full path to the file to open
 *  @return file id: non-negative integer or -1 in case of error
*/
uint32_t open_file(const char *filename);
