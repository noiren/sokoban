@echo off
echo Starting GBA Sokoban Audio SE Editor...
python ..\..\..\src\gameeditor\audio_se_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
