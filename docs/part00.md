# Part 0 - Setting Up

## Prior knowledge
This tutorial assumes that you know how to code in C++, specifically C++17. It assumes that you know what's a good compiler for your system, and how to install it. We'll make a few references to git early on, but won't remind you to commit your code. I work on a Linux system, so that's what I know. This code is tested to compile on Windows, but might not fit your workflow.

## Setup
Versions of this tutorial in other languages make comments about how there isn't much to do in terms of setup. This tutorial is different. Why? Because C++ has build systems and the package managers require some work. We actually have a fair amount to do, so let’s get started.

First thing we're going to do is create a directory for our project. I've called mine yarl (as in Yet Another RogueLike). And then we'll run
```bash
git init
```
Do we need to have our code in a git repo? No.

Do we need to have it in version control? I see you like to live dangerously.

So why are we doing this? Our package manager kind of assumes we're working in a git repo. So that's what I'll talk about (unless I get enough complaints).

Next run the command
```bash
git submodule add https://github.com/microsoft/vcpkg
```
This clones vcpkg (our package manager) into a submodule. This is their recommendation for how to use it l. Next we run the command 
```
./vcpkg/bootstrap-vcpkg.sh
```
This should install vcpkg in a place where cmake can find it. (cmake? No one said anything about cmake. We'll come back to this)

Next we'll specify our external dependencies in `vcpkg.json`
```json
{
  "dependencies": [
    {
      "name": "libtcod",
      "version>=": "2.1.1"
    },
    {
      "name": "flecs",
      "version>=": "4.1.0"
    },
    {
      "name": "sdl3",
      "version>=": "3.2.16"
    }
  ]
}
```
Okay let's talk about these dependencies. [libtcod](https://github.com/libtcod/libtcod) is a fairly common library for writing roguelikes. It's primarily written in C, but has a C++ wrapper. It also has a python wrapper that is very popular. I've chosen version 2.1.1 because it's the latest at the time I'm writing this. You’ll definitely want at least version 2.0 as that has breaking changes.
Next is [flecs](https://www.flecs.dev/flecs/md_docs_2DesignWithFlecs.html). This is an ECS written in C with a C++ wrapper. The current trend in roguelikes is to use ECS for composition (as opposed to inheritance). Again, 4.1.0 is the latest version, and you should use at least that one as it had [breaking changes](https://ajmmertens.medium.com/flecs-4-1-is-out-fab4f32e36f6).
Finally [SDL](https://wiki.libsdl.org/SDL3/FrontPage) is a highly portable development library designed to allow access to hardware, including mouse, keyboard, and graphics. SDL3 was a major change, and using version 3 is why we need libtcod 2.

Now we'll run this
```
./vcpkg/vcpkg x-update-baseline --add-initial-baseline
```
This will add a baseline to our vcpkg.json. For me it did
```diff
-   ]
+  ],
+  "builtin-baseline": "bffcbb75f71553824aa948a7e7b4f798662a6fa7"
}
```
Your baseline might be a different hash. That's okay. We just need it to be there for vcpkg to work.

Next we need to set up cmake. Everybody has their favorite build system. I don't think anyone's is cmake. It's not mine. But cmake is cross-platform which works for a game that we'd like to easily release on multiple platforms. Or a tutorial where people reading it will be using different platforms to follow along. For cmake, we need the file CMakeLists.txt in our repo’s root directory. It should contain the following
```cmake
cmake_minimum_required (VERSION 3.23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the default toolchain to use a Vcpkg submodule.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(
    yarl
    LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")  # Save all runtime files to this directory.

file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS  # Automatically reconfigure if source files are added/removed.
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
)

# Enable warnings recommended for new projects.
if (MSVC)
    add_compile_options(/W2)
else()
    add_compile_options(-Wall -Wextra -Wconversion -Werror -pedantic -g)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Ensure the C++17 standard is available.
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Enforce UTF-8 encoding on MSVC.
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(libtcod CONFIG REQUIRED)
find_package(flecs)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 libtcod::libtcod flecs::flecs_static)
```
Okay. There's a lot going on here, and to be completely honest I understand very little of cmake. This was stolen from another one of my projects, and tweaked for this. If I get questions about how this particular cmake file, I'll attempt to answer them here.

One other thing we're going to need is a font. We'll just use the one from the python tutorial. I'll put it here so you don't have to go searching for it.
![](img/dejavu10x10_gs_tc.png)
Make a directory called assets, and put it in there.

We're 800 words into a C++ tutorial and we haven't written any C++, let alone compiled anything. Let's change that. Here's very minimal code that will let us test that everything is set up properly.
```cpp src/main.cpp
#include <SDL3/SDL.h>
#include <flecs.h>
#include <libtcod.hpp>

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) { return 0; }
```
You may be saying to yourself “But, that doesn't do anything.” And you’d be right, kind of. It does nothing at runtime. What it does do is include a header from each of our dependencies so we can ensure that they're actually installed. You might also be asking yourself “What are those `[[maybe_unused]]`?” Well, one of the warnings/errors we turned on is for unused variables. It's generally a good idea, but we aren't actually using them. So we're telling the compiler that it's okay and we know we aren't using them. Astute C++ (or C) developers might ask why don't we just use the other type signature for main. Well, that's because SDL defines its own main and has a macro to change the name of yours.

Now that we've written some actual code, it's time to compile it, cmake style. First, we need to tell cmake to build all its personal requirements. We do this with the commands
```bash
mkdir build
cmake -S . -B build
```
This puts a ton of stuff that we don't need to think about in the build directory, and probably takes a while. Go get a coffee. Or a tea. Or play a run of [Rapid Brogue](https://github.com/flend/RapidBrogue) and come back later. Once that finishes successfully, our command to build our code is
```bash
cmake --build build
```
