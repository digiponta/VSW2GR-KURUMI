#!/bin/sh

 rmmod ftdi_sio
 rmmod usbserial

./vsw2gr-kurumi-static aaa < /tmp/kurumi_sketch.bin
  
