#pragma once

#include <memory>
#include <string>
#include <vector>

#include "config.h"
#include "exception.h"
#include "options.h"
#include "path.h"
#include "process.h"
#include "toml.h"
#include "utils.h"

#ifdef _WIN32

#define OBJ_EXT ".obj"
#define BIN_EXT ".exe"

#else

#define OBJ_EXT ".o"
#define BIN_EXT ""

#endif

struct Build
{
    static void run(const BuildOptions* options);
    static void build(BuildEnvironment& env);

private:
    static void compile(BuildEnvironment& env, const Path& file);
    static void link(BuildEnvironment& env, const Path& file);

    static std::string compileCommand(const BuildEnvironment& env, const Path& file, const Path& object);
    static std::string linkCommand(const BuildEnvironment& env, const Path& file);

    static bool shouldUpdate(const BuildEnvironment& env, const Path& file, const Path& object);
    static void updateCache(const BuildEnvironment& env, const Path& file);

    static Path cachePath(const BuildEnvironment& env, const Path& file);

};
