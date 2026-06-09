set PATH=C:\Qt\Tools\llvm-mingw1706_64\bin;C:\Qt\6.11.0\llvm-mingw_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;%PATH%
set CC=clang
set CXX=clang++
cd /d C:\Users\juneh\Documents\QtProjects\pokered-save-editor-2
rmdir /s /q build
C:\Qt\Tools\CMake_64\bin\cmake.exe -S projects -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/llvm-mingw_64 -DPSE_BUILD_TESTS=ON > _ci.log 2>&1
echo CONFIGURE_DONE_%ERRORLEVEL% >> _ci.log
C:\Qt\Tools\CMake_64\bin\cmake.exe --build build >> _ci.log 2>&1
echo BUILD_DONE_%ERRORLEVEL% >> _ci.log
