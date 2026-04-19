This simple program provides histogram plotting of the likelihood of possible outcomes in a Risk battle. 
The Monte Carlo simulation is performed in C++, Pybind11 is used to make this functionality available in Pyhon which is used to handle plotting.

Maximum overengineering is accomplished by having montecarlo simulations performed in parallel on all available hardware threads.