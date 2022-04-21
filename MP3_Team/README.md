
Compile the code using the provided makefile:

    make

To clear the directory (and remove .txt files):
   
    make clean

To run the Coordinator on port 8080:

    ./coordinator -p 8080

To run the Master/Slaves:  

    ./tsd -h 0.0.0.0 -c 8080 -p 1260 -i 1 -t master
    ./tsd -h 0.0.0.0 -c 8080 -p 1261 -i 1 -t slave
    ./tsd -h 0.0.0.0 -c 8080 -p 1262 -i 2 -t master
    ./tsd -h 0.0.0.0 -c 8080 -p 1263 -i 2 -t slave
    ./tsd -h 0.0.0.0 -c 8080 -p 1264 -i 3 -t master
    ./tsd -h 0.0.0.0 -c 8080 -p 1265 -i 3 -t slave
    
To run the Clients:

    ./tsc -h 0.0.0.0 -c 8080 -p 1266 -i 1
    ./tsc -h 0.0.0.0 -c 8080 -p 1267 -i 2
    ./tsc -h 0.0.0.0 -c 8080 -p 1268 -i 3
    ./tsc -h 0.0.0.0 -c 8080 -p 1269 -i 4
    ./tsc -h 0.0.0.0 -c 8080 -p 1270 -i 5
    ./tsc -h 0.0.0.0 -c 8080 -p 1271 -i 6
    
To run the Follow Synchronizers:

    ./synchronizer -h 0.0.0.0 -c 8080 -p 1272 -i 1
    ./synchronizer -h 0.0.0.0 -c 8080 -p 1273 -i 2
    ./synchronizer -h 0.0.0.0 -c 8080 -p 1274 -i 3
