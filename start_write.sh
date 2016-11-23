#!/bin/sh

PATH1=`modinfo usbserial | grep filename | sed -e 's/filename:[ ]*//'`
PATH2=`modinfo ftdi_sio | grep filename | sed -e 's/filename:[ ]*//'`

echo $PATH1
echo $PATH2

rmmod ftdi_sio
rmmod usbserial

./vsw2gr-kurumi-static aaa < $1
  
insmod ${PATH1}
insmod ${PATH2}
