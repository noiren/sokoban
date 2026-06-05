@echo off
echo Starting GBA Sokoban Puzzle Event Editor...
python ..\..\..\src\gameeditor\puzzle_event_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
