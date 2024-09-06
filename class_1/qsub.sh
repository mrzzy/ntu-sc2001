#!/bin/bash
#PBS -N sc2001_class_1
#PBS -l select=1:ncpus=128:mem=256gb
#PBS -l walltime=24:00:00
#PBS -o sc2001_class_1.log
#PBS -j oe
#PBS -q normal

set -ex -o pipefail
cd $PBS_O_WORKDIR

# install pypy
curl https://downloads.python.org/pypy/pypy3.10-v7.3.17-linux64.tar.bz2 | tar xjf -
export PATH="$PBS_O_WORKDIR/pypy3.10-v7.3.17-linux64/bin:$PATH"

time pypy3 code.py
