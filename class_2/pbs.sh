#!/bin/bash
#PBS -N sc2001_class2
#PBS -l select=1:ncpus=128:mem=440G
#PBS -l walltime=1:00:00
#PBS -j oe
#PBS -o out.log
#PBS -q normal

set -ex -o pipefail

cd $PBS_O_WORKDIR
module load python/3.10.9
source .venv/bin/activate
jupyter nbconvert --to script experiments.ipynb
python experiments.py
