#! /bin/sh
debug_dir=`pwd`
base=${debug_dir%/*}
make -f $base/src/vmachine/makefile
make -f $base/src/vmachine/makefile
cp $base/src/vmachine/vmachine $debug_dir/
cp $base/src/tty/tty $debug_dir/
echo compile ok.
echo start to debug.
$debug_dir/vmachine | tee debug.log &
$debug_dir/tty < debug.dat &
$debug_dir/tty < debug2.dat
exit
