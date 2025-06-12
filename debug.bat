@echo off
cls

Rem Build
cmake -S . -B Build\\Debug -G "MinGW Makefiles" -D CMAKE_BUILD_TYPE=Debug
cmake --build Build\\Debug --config Debug
cd Build\\Debug\\Run
move /y ..\\compile_commands.json ..\..

Rem Lauch
MyProject.exe
cd ..\..\..

