@echo off
echo Starting GBA Sokoban UI Editor...
python ..\..\..\src\gameeditor\ui_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
