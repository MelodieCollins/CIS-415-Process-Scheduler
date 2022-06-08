# CIS-415-Process-Scheduler
This program initiates and schedules a workload of other processes. This User Space Process 
Scheduler (USPS, not to be confused with the US Postal Service) reads a list of programs, 
with arguments, from a file specified as an argument to the scheduler (uses standard input if unspecified), 
starts up the programs in processes, and then schedules the processes to run concurrently in a time-sliced 
manner. It will also monitor the processes, keeping track of how the processes are using system 
resources. 
This is a good introduction to processes, signals and signal handling, and scheduling. 
