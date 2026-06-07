@echo off
echo Starting GBA Sokoban Text Data Editor...
python ..\..\..\src\gameeditor\text_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
