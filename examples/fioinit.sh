#!/bin/bash

if [ $# -lt 3 ]; then
  echo "Usage: $0 [total size in gb] [number of jobs] [filepath]"
  exit 0
fi


admin_addr=$1
instances=$2

totalsizegb=$1
numjobs=$2
filename=$3
jobfile="./hcd-init.fio"

echo "will generate fio job file $jobfile"

echo "[global]
ioengine=libaio
direct=1
filename=$filename
group_reporting
" > $jobfile

gb_per_job=$(($totalsizegb/$numjobs))
offset=0

for ((i = 0; i < $numjobs; i++)); do
  echo >> $jobfile
  echo "[part$i]" >> $jobfile
  echo "rw=write" >> $jobfile
  echo "numjobs=1" >> $jobfile
  echo "bs=128k" >> $jobfile
  echo "offset=${offset}g" >> $jobfile
  echo "size=${gb_per_job}g" >> $jobfile
  offset=$(($offset + $gb_per_job))
done
