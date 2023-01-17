#!/bin/bash

RUN_START="1"
RUN_STOP="100"

CURRENT_RUN_START="1"
CURRENT_RUN_STOP="100"

SLIP_BETWEEN_SIMS="1m"

# 1=OLSR; 2=AODV; 3=DSDV; 4=DSR
ROUTING="2"

# Number of nodes
NODES="50"

# Number of source nodes - nodes that send packets
N_SOURCE_NODES="10"

# Data rate: 1kbps - controll traffic, 10kbps - high controll traffic, 1000kbps - video traffic
RATES="4kbps"

# Size of packets (payload size) in bytes: 64B for small controll packets, 2048B for large video packets
PACKET_SIZE="512"

# Scenario: 0 = Random Waypoint model, 1 = Manhattan Grid from NS-2 trace (ns2Trace-<broj cvorova>.txt)
SCENARIO=1

# Name of the script (.cc file in the scratch folder) - use different files for different scenarios
PROGRAM_NAME="vanet-npaf"

echo Experiment starts...

for r in $ROUTING
do
  for node in $NODES
  do
    for source in $N_SOURCE_NODES
    do
      for rate in $RATES
      do
        for (( run=$CURRENT_RUN_START; run<=$CURRENT_RUN_STOP; run++ ))
        do
          echo 
          echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
          echo x   Run = $run
          echo x   "./ns3 run \"$PROGRAM_NAME --scenario=$SCENARIO --routingProtocol=$r (protocol: 1=OLSR; 2=AODV; 3=DSDV; 4=DSR)"
          echo x   "      --nNodes=$node --nSources=$source --dataRate=$rate --packetSize=$PACKET_SIZE"
          echo x   "      --startRngRun=$RUN_START --currentRngRun=$run --stopRngRun=$RUN_STOP\""
          echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
          echo 
          echo "Start time:"

          date
          ./ns3 run "$PROGRAM_NAME --scenario=$SCENARIO --routingProtocol=$r --nNodes=$node --nSources=$source --dataRate=$rate --packetSize=$PACKET_SIZE --startRngRun=$RUN_START --currentRngRun=$run --stopRngRun=$RUN_STOP" 
          echo "StopTime:"
          date
          
          echo --------------------------------------------------------------------------
          
          spd-say "End of run $run."

          echo Sleeping now for $SLIP_BETWEEN_SIMS...
          sleep $SLIP_BETWEEN_SIMS
        done
      done
    done
  done  
done

