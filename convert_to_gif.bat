@echo off
setlocal enabledelayedexpansion

echo Converting mp4 files in 'video' folder to GIF...
mkdir media 2>nul

set count=1
for %%f in (video\*.mp4) do (
    echo Converting %%f to media\gif!count!.gif
    ffmpeg -y -i "%%f" -vf "fps=15,scale=600:-1" "media\gif!count!.gif"
    set /a count+=1
)

echo Done! The GIFs are in the 'media' folder.
pause
