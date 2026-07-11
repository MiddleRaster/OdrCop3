rmdir /S /Q .\llvm\include\llvm
rmdir /S /Q .\llvm\include\llvm-c
rmdir /S /Q .\llvm\include\clang

xcopy /S /Y ..\llvm-project-main\llvm\include\llvm     .\llvm\include\llvm\
xcopy /S /Y ..\llvm-project-main\llvm\include\llvm-c   .\llvm\include\llvm-c\
xcopy /S /Y ..\llvm-project-main\clang\include\clang-c .\llvm\include\clang-c\
xcopy /S /Y ..\llvm-project-main\clang\include\clang   .\llvm\include\clang\

xcopy /S /Y C:\llvm-build\include\llvm                 .\llvm\include\llvm\
xcopy /S /Y C:\llvm-build\tools\clang\include\clang    .\llvm\include\clang\
xcopy    /Y C:\llvm-build\lib\*.lib                    .\llvm\lib\
