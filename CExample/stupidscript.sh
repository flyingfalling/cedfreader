#!/bin/bash
END=10

for idx in $(seq 1 $END); do 
    ./edf2asc ~/richard_home/git/cedfreader/SUB001.EDF -fvel -y
    cp ~/richard_home/git/cedfreader/SUB001.asc "rep_""$idx"".asc"
done
