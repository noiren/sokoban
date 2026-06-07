@echo off
echo Starting GBA Sokoban Asset Maker (Portrait / Bake)...
python ..\..\..\src\gameeditor\portrait_maker.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo An error occurred. Please check the python installation or the script.
    pause
)
