#!/bin/bash -e
export PATH=$PATH:/Users/jakub/dev/priv/mdec-tools/cmake-build-relwithdebinfo
ffmpeg -hide_banner -loglevel panic -i bad_apple.mkv -ss 2 -t 00:00:25 -vf "fps=10,scale=256:240" -pix_fmt gray %04d.png
mdec-tools -q=40 --color --movie *.png bad_apple.h
rm *.png
