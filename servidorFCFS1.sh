#!/bin/bash
make clean
make
rm output-folder/output

./bin/orchestrator output-folder 1 FCFS