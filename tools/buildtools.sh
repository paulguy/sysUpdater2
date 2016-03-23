#!/bin/sh

cc -o toctrfb toctrfb.c `pkg-config --cflags --libs MagickWand`
