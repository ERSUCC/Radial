#pragma once

#include <iostream>
#include <filesystem>
#include <stddef.h>
#include <stdio.h>
#include <string>

#include "build.h"
#include "config.h"
#include "exception.h"
#include "options.h"
#include "utils.h"

struct Run
{
    static void run(const RunOptions* options);
};
