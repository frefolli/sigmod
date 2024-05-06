#!/bin/bash

rm -rf .reprozip-trace/ submission.rpz
reprozip trace bash ./run.sh
reprozip pack submission.rpz
