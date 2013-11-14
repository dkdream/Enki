#!/bin/sh

set -x

grep -h SUBR buildins/*.c |\
 sed -e 's/$/;/' |\
 grep -v '#define' >buildins/SUBR.lst
