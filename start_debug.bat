@echo off
echo mGBA をGDBスタブ付きで起動します (port 3333)...
echo VSでブレークポイントを設定したら、デバッグ構成 "GBA: mGBA GDB デバッグ" を実行してください。
start "" "C:\Program Files\mGBA\mGBA.exe" -g "C:\Users\ryota\.gemini\antigravity\scratch\gba-sokoban-bn\gba-sokoban-bn.gba"
