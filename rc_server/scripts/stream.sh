#!/bin/bash

trap 'kill -9 $(jobs -p)' SIGTERM

source /home/aeinstein/.bashrc
source /home/aeinstein/.profile

export PULSE_RUNTIME_PATH="/run/user/$(id -u)/pulse/"
export XDG_RUNTIME_DIR="/run/user/$(id -u)"

APP=$1
STREAM=$2

echo "$1 stream $2 start" >> /tmp/stream.log

# stream libcam h264 -> ffmpeg copy -> rtmp server
#libcamera-vid -t 0 -g 10 --codec h264 --inline -n --flush -o - \
#ffmpeg -i - -vcodec copy -f flv rtmp://127.0.0.1:1935/demo/live


# Streaming libcam raw yuv420 -> ffmpeg h264 -> rtmp server
/usr/bin/libcamera-vid -t 0 -g 10 --codec yuv420 --inline -n --flush -o - | \
/usr/bin/ffmpeg -f rawvideo -vcodec rawvideo -s 640x480 -r 30 -pix_fmt yuv420p -rtbufsize 1M -i - -f pulse -i default \
-vf drawtext="fontfile=monofonto.ttf: fontsize=20: box=1: boxcolor=black@0.75: boxborderw=5: fontcolor=white: x=(w-text_w)/2: y=((h-text_h)/2)+((h-text_h)/4): text='%{gmtime\:%M\\\\\:%S}'" \
-vcodec libx264 -b:v 2M -s 640x480 -preset ultrafast -profile:v main -x264-params keyint=30:scenecut=0 -acodec aac -ac 1 -ar 44100 -shortest -f flv rtmp://127.0.0.1/"$APP"/"$STREAM" &

wait

# stream libcam libav -> rtmp server

#-x264-params keyint=120:min-keyint=30:scenecut=0 -movflags +faststart
#libcamera-vid -t 0 -g 10 --codec libav --libav-video-codec libx264 --inline -n --flush -o rtmp://127.0.0.1:1935/demo/live

