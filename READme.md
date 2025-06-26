To run:
g++ Main.cpp -o main
./main

#### config.txt

The `config.txt` file is the settings file for the operating systems emulator. Instead of hardcoded values in the program, the configurations for the system comes from this file. 

Contents of `config.txt`
Based on the specifications, here are the required parameters of `config.txt`

num-cpu - description of cpu cores 
scheduler - scheduling algorithm
quantum-cycles - time slice for round robin
batch-process-freq - how often to generate processes
min-ins - minimum instructions per process
max-ins - maximum instructions per process
delayes-per-exec - delays between instructions 

Example:
4
rr
5
4
10
50
1

or 

num-cpu 4
scheduler rr
quantum-cycles 5
batch-process-freq 3
min-ins 10
max-ins 50
delays-per-exec 1


