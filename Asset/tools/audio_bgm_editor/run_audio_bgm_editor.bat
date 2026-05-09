@echo off
echo Starting GBA Sokoban Audio BGM Editor...
python ..\..\..\src\tools\audio_bgm_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
