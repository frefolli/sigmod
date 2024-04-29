#!/bin/bash

rm -rf spg.tar.gz
mkdir spg
cp plots/*.csv spg/
tar cvf spg.tar spg
gzip spg.tar
rm -rf spg/
