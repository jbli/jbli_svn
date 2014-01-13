#! /bin/sh

export PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin
USED_SWAP=$(tail -1 /proc/swaps|awk '{print $(NF-1)}')
if [ $USED_SWAP -gt 1200000 ]
then
    NOW=$(date "+%Y/%m/%d %H:%M")
    /bin/echo "$NOW  used_swap:$USED_SWAP, clear cache" >> /tmp/clearcache.log 
    /bin/sync && /bin/echo 3 > /proc/sys/vm/drop_caches
    sleep 45
    /bin/echo 0 > /proc/sys/vm/drop_caches
fi