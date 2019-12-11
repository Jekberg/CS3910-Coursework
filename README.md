# CS3910-Coursework

## How to build

To build the project, the tool CMake is required... It shouldn't be hard to find
but here is a link to their website https://cmake.org/. CMake should be easy to
find on a *Nix system. The second thing required is ANY (almost) C++ compiler
which supports the C++ 17 standatd, I used MSVC and GCC myself.

If the project is being built on a windown computer, the best option is to open
the project folder using Visual Studio 2019 by doing "File > Open > Folder",
this will allow VS to run the appropriate code to generate a builld system
automatically using the CMake file... however, the CMake and C++ features MUST
be installed as part of VS
(This should be trivial as it can be installed at any point). The default build
is Debug which may run a bit slow, but this can be changed right next to the
"Run" button.

If the project is being built on Linux (Or maybe Mac), the run the command`cmake -S. -B./build -DCMAKE_BUILD_TYPE=Release`, this will generate a release
build system. Then run `cmake --build build` to build the two solutions
(There shouldn't be any errors). The executables should be placed in the
`./build/bin` directory. The linux/mac solutions may require the TBB library,
but this may ship as part of the platform?

## How to run

...

## Particle Swarm Optimisation
To view the best result from each iteration go to line 228 in PSO.h and uncomment the lines of code.
The output is a bit mangled since there may be 2 levels of PSO running...

## Genetic Programming
To view the best result from each iteration go to line 258 in GP-Main.cpp and uncomment the lines of code.
