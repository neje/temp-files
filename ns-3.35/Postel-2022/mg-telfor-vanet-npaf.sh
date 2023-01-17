#!/bin/bash

START="1"
STOP="200"

CURRENT_RUN_START="1" 
CURRENT_RUN_STOP="200" 

SLIP_BETWEEN_SIMS="10s"

# 1=OLSR; 2=AODV; 3=DSDV; 4=DSR
ROUTING="2"

# Number of nodes 
#N_NODES="100 150 200 250 300 350"
N_NODES="50"

# Number of source nodes - nodes that send packets
N_SOURCE_NODES="10"

# Data rate: 1kbps - controll traffic, 10kbps - high controll traffic, 1000kbps - video traffic
RATE="4kbps"

# Size of packets (payload size) in bytes: 64B for small controll packets, 2048B for large video packets
PACKET_SIZE="512" #[B]

# Name of the script (.cc file in the scratch folder) - use different files for different scenarios
PROGRAM_NAME="mg-telfor-vanet-npaf"

# Scenario
# 0 - RW 15m/s, 
# 1 - MG 2x2km sa semaforom u sredini na 500x500m (jedna traka, 15m/s), 
# 2 - MG 2x2km bez semafora, 
# 3 - MG 1x1km sa semaforima na svakoj raskrsnici (i dve trake i manjom brzinom 13.89m/s)
SCENARIO="1" 

echo Experiment starts...


for n in $N_NODES
do
  for (( run=$CURRENT_RUN_START; run<=$CURRENT_RUN_STOP; run++ ))
  do
    echo 
    echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    echo x   Run = $run
    echo x   "./waf --run \"$PROGRAM_NAME --routingProtocol=$ROUTING"
    echo x   "      --nNodes=$n --nSources=$N_SOURCE_NODES --dataRate=$RATE --packetSize=$PACKET_SIZE"
    echo x   "      --startRngRun=$START --currentRngRun=$run --stopRngRun=$STOP --scenario=$SCENARIO"
    echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    echo 
    echo "Start time:"
    date
    ./waf --run "$PROGRAM_NAME --routingProtocol=$ROUTING --nNodes=$n --nSources=$N_SOURCE_NODES --dataRate=$RATE --packetSize=$PACKET_SIZE --startRngRun=$START --currentRngRun=$run --stopRngRun=$STOP --scenario=$SCENARIO" 
    echo "StopTime:"
    date
    echo --------------------------------------------------------------------------
    echo Sleeping now for $SLIP_BETWEEN_SIMS...
    sleep $SLIP_BETWEEN_SIMS
  done
done


