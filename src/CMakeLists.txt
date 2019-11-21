add_executable(
    "PSO-EXE"
    "PSO-Main.cpp")

target_include_directories(
    "PSO-EXE"
    PRIVATE
        ${CS3910_INCLUDE_DIR})

target_link_libraries(
    "PSO-EXE"
    PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:tbb>)

add_executable(
    "GP-EXE"
    "GP-Main.cpp")

target_include_directories(
    "GP-EXE"
    PRIVATE
        ${CS3910_INCLUDE_DIR})

target_link_libraries(
    "GP-EXE"
    PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:tbb>)