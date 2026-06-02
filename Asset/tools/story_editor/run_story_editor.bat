@echo off
echo Starting GBA Sokoban Story Editor...
python ..\..\..\src\gameeditor\story_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
