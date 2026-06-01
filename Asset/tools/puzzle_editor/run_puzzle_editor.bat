@echo off
echo Starting GBA Sokoban Puzzle Editor...
python ..\..\..\src\gameeditor\puzzle_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
