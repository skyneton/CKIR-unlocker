#pragma once
#include "privilege.h"
#include "color.h"
#include <Windows.h>
#include<vector>
#include "Processor.h"

#define BUILD_TYPE_NORMAL 0
#define BUILD_TYPE_FORCE 1
#define BUILD_TYPE_RESUME 2
#define BUILD_TYPE_WORM 3
#define BUILD_TYPE_LOG_PROC 4

void run(int buildType);