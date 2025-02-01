#!/bin/bash

#libcamera-vid -t 0 -g 10 --codec h264 --inline -n --flush -o - \
#ffmpeg -i - -vcodec copy -f flv rtmp://127.0.0.1:1935/demo/live


libcamera-vid -t 0 -g 10 --codec yuv420 --inline -n --flush -o - | \
ffmpeg -f rawvideo -vcodec rawvideo -s 640x480 -r 30 -pix_fmt yuv420p -i - -f pulse -i default \
-vf drawtext="fontfile=monofonto.ttf: fontsize=20: box=1: boxcolor=black@0.75: boxborderw=5: fontcolor=white: x=(w-text_w)/2: y=((h-text_h)/2)+((h-text_h)/4): text='%{gmtime\:%M\\\\\:%S}'" \
-vcodec libx264 -b:v 2M -s 640x480 -preset ultrafast -profile:v main -x264-params keyint=120:scenecut=0 -acodec aac -ac 1 -ar 44100  -f flv rtmp://127.0.0.1/demo/live

#-x264-params keyint=120:min-keyint=30:scenecut=0 -movflags +faststart
