#!/bin/bash

rm -rf transfer.tar.gz
python3 -m plots.lsh
mkdir transfer
mv *.png transfer
tar cvf transfer.tar transfer
gzip transfer.tar
rm -rf transfer
