add_executable("PSO-EXE" "Main.cpp")

target_include_directories("PSO-EXE" PRIVATE ${CS3910_INCLUDE_DIR})

target_link_libraries("PSO-EXE" PRIVATE tbb)