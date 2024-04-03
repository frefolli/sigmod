# Dummy

Processing di 100 query con uso di C_map

## KD Tree

./builddir/main.exe
# TIME | Read database, length = 10000 | el. 2 ms
# TIME | Read query_set, length = 100 | el. 0 ms
# TIME | Indexes Database | el. 2 ms
# TIME | Built KD Forest | el. 4 ms
# TIME | Used KD Forest | el. 85 ms
# TIME | Freed KD Forest | el. 0 ms
# TIME | Freed DB&QS | el. 0 ms
# ETA (ms): 96

## Ball Tree

./builddir/main.exe
# TIME | Read database, length = 10000 | el. 2 ms
# TIME | Read query_set, length = 100 | el. 0 ms
# TIME | Indexes Database | el. 4 ms
# TIME | Built Ball Forest | el. 23 ms
# TIME | Used Ball Forest | el. 69 ms
# TIME | Freed Ball Forest | el. 0 ms
# TIME | Freed DB&QS | el. 0 ms
# ETA (ms): 102

## Exaustive

./builddir/main.exe
# TIME | Read database, length = 10000 | el. 3 ms
# TIME | Read query_set, length = 100 | el. 0 ms
# TIME | Indexes Database | el. 2 ms
# TIME | Used Exaustive | el. 39 ms
# TIME | Wrote Solution | el. 1 ms
# TIME | Freed DB&QS | el. 1 ms
# ETA (ms): 49

# Contest 1M

Processing di 1000 query con uso di C_map

## KD Tree

./builddir/main.exe contest-1m-data.bin contest-1m-queries.bin
# TIME | Read database, length = 1000000 | el. 213 ms
# TIME | Read query_set, length = 10000 | el. 3 ms
# TIME | Indexes Database | el. 625 ms
# TIME | Built KD Forest | el. 1143 ms
# TIME | Used KD Forest | el. 157257 ms
# TIME | Freed KD Forest | el. 0 ms
# TIME | Freed DB&QS | el. 91 ms
# ETA (ms): 159335

## Ball Tree

./builddir/main.exe contest-1m-data.bin contest-1m-queries.bin
# TIME | Read database, length = 1000000 | el. 201 ms
# TIME | Read query_set, length = 10000 | el. 3 ms
# TIME | Indexes Database | el. 556 ms
# TIME | Built Ball Forest | el. 8011 ms
# TIME | Used Ball Forest | el. 4374 ms
# TIME | Freed Ball Forest | el. 4 ms
# TIME | Freed DB&QS | el. 103 ms
# ETA (ms): 13256

## Exaustive

./builddir/main.exe contest-1m-data.bin contest-1m-queries.bin
# TIME | Read database, length = 1000000 | el. 204 ms
# TIME | Read query_set, length = 10000 | el. 2 ms
# TIME | Indexes Database | el. 585 ms
# TIME | Used Exaustive | el. 37628 ms
# TIME | Wrote Solution | el. 3 ms
# TIME | Freed DB&QS | el. 93 ms
# ETA (ms): 38517

# Contest 10M

Processing di 1000 query con uso di C_map

## KD Tree

./builddir/main.exe contest-10m-data.bin contest-10m-queries.bin
# TIME | Read database, length = 10000000 | el. 6168 ms
# TIME | Read query_set, length = 1000000 | el. 731 ms
# TIME | Indexes Database | el. 9114 ms
# TIME | Built KD Forest | el. 24896 ms
# TIME | Used KD Forest | el. 1819900 ms
# TIME | Freed KD Forest | el. 20 ms
# TIME | Freed DB&QS | el. 1006 ms
# ETA (ms): 1861847

## Ball Tree

./builddir/main.exe contest-10m-data.bin contest-10m-queries.bin
# TIME | Read database, length = 10000000 | el. 5090 ms
# TIME | Read query_set, length = 1000000 | el. 663 ms
# TIME | Indexes Database | el. 7572 ms
# TIME | Built Ball Forest | el. 98824 ms
# TIME | Used Ball Forest | el. 8714 ms
# TIME | Freed Ball Forest | el. 33 ms
# TIME | Freed DB&QS | el. 821 ms
# ETA (ms): 121728

## Exaustive

./builddir/main.exe contest-10m-data.bin contest-10m-queries.bin
# TIME | Read database, length = 10000000 | el. 5070 ms
# TIME | Read query_set, length = 1000000 | el. 493 ms
# TIME | Indexes Database | el. 7660 ms
# TIME | Used Exaustive | el. 298525 ms
# TIME | Wrote Solution | el. 2943 ms
# TIME | Freed DB&QS | el. 1150 ms
# ETA (ms): 315847
