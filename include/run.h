#pragma once

#include <filesystem>
#include <string>

#include "build.h"
#include "config.h"
#include "options.h"
#include "process.h"
#include "utils.h"

struct Run
{
    static void run(const RunOptions* options);
};
