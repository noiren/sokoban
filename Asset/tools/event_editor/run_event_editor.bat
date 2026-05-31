@echo off
echo Starting GBA Sokoban Event Script Editor...
python ..\..\..\src\gameeditor\event_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
