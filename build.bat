@echo off
make
if %ERRORLEVEL% EQU 0 (
    echo Build successful. Copying ROM to binary folder...
    if not exist binary mkdir binary
    copy /Y gba-sokoban-bn.gba binary\gba-sokoban-bn.gba
) else (
    echo Build failed.
)
