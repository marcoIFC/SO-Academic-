#!/bin/bash
for((i=0; i<4; i++))
do
./bin/client execute 300 -p "ls -l|wc"
./bin/client execute 100 -u "progteste/void 1"
./bin/client execute 1000 -u "progteste/hello 1"
./bin/client status
./bin/client execute 500 -p "ls -l|wc"
done