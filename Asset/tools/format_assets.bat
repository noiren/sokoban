@echo off
echo ===================================================
echo   GBA Sokoban - Asset Formatter
echo ===================================================
echo This tool will automatically format images in Asset\graphics\stills\
echo  - Converts .png to .bmp
echo  - Renames uppercase letters to lowercase
echo  - Pads images (e.g. 240x160) to 256x256 with black background
echo  - Converts to 8-bit indexed color (256 colors)
echo.

python "%~dp0format_assets.py"

echo.
pause
