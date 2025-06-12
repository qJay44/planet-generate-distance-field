@echo off
cls

Rem Build
cmake -S . -B Build\\Release -G "MinGW Makefiles" -D CMAKE_EXPORT_COMPILE_COMMANDS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build Build\\Release --config Release
cd Build\\Release\\Run
move /y ..\\compile_commands.json ..\..

Rem Lauch
MyProject.exe
cd ..\..\..

