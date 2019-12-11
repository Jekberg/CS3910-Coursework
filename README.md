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

The solutions can be executed from the root project directory by simply listing the path to the executable from the root of the project (Staring with "./build/bin" (UNLESS Windows...)). By default the solution will look for the "./sample" folder which contains the csv data files. Running the project in VS will likely error because it wants to launch the project from some weird location... but running it from powershell or CMD should be easy, almoste just like it is done on Unix. Windows will generate an "out" folder where the solution is placed depending on if the project was built in release mode or debug mode.

The executables accept 2 optional arguments, these are the relative path to the file containing thetraining data and test data in that order...

Oh! By the way, the executables to run are called "PSO-EXE" and "GP-EXE"-

### Particle Swarm Optimisation
The PSO can be tweaked by changing the code in the PSO-Main.cpp file and the PSO.h file.

To view the best result from each iteration go to line 228 in PSO.h and uncomment the lines of code.
The output is a bit mangled since there may be 2 levels of PSO running...

### Genetic Programming
The GP can be tweaked by changing the code in the GP-Main.cpp file and the GP.h file.

To view the best result from each iteration go to line 258 in GP-Main.cpp and uncomment the lines of code.
