#!/bin/sh

# visual_err can be found at https://github.com/JohnnyonFlame/visual-err

rm -rf opk hexen2.opk
mkdir opk
mkdir opk/data1
cp hexen.rc gcw0_default.cfg opk/data1
cp ../hexen2/hexen2 visual_err hexen2.gcw0.desktop hexen2.png hexen2-launch opk/
mipsel-linux-strip opk/*
chmod +x opk/visual_err opk/hexen2 opk/hexen2-launch
mksquashfs opk/ hexen2.opk
rm -rf opk