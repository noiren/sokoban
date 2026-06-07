@echo off
echo Starting GBA Sokoban Gallery Editor...
python ..\..\..\src\gameeditor\gallery_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
