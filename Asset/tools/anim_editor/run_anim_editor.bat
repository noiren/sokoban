@echo off
echo Starting GBA Anim Editor...
python ..\..\..\src\tools\anim_editor.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
