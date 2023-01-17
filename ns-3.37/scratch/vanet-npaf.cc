#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>    
#include <vector>

#include "ns3/core-module.h"
#include "ns3/nstime.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/random-variable-stream.h"

#include "ns3/aodv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"

#include "ns3/npaf-module.h"
#include "ns3/netanim-module.h"

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

using namespace ns3;
using namespace npaf;

NS_LOG_COMPONENT_DEFINE ("VanetExample");

void
PrintCurrentTime ()
{
  std::ofstream file;
  double time = Simulator::Now ().GetSeconds ();
  if (time == 0.0)
    file.open ("current-status.txt", std::ios::trunc);
  else
    file.open ("current-status.txt", std::ios::app);
  file << time << " s\n";
  file.close ();
  Simulator::Schedule (Seconds (1), &PrintCurrentTime);
}

/////////////////////////////////////////////
// class RoutingExperiment
// controls one program execution (run), holds data from current run
/////////////////////////////////////////////
class RoutingExperiment
{
public:
  RoutingExperiment (uint64_t stopRun = 1, std::string fn = "IJTTE"); // default is only one simulation run
  RoutingExperiment (uint64_t startRun, uint64_t stopRun, std::string fn = "IJTTE");
  RunSummary Run (int argc, char **argv);
  void WriteToSummaryFile (RunSummary srs);
  void SetSimDuration (double simDur) { m_simDuration = simDur; };

private:
  uint64_t m_startRngRun; // first RngRun
  uint64_t m_stopRngRun; // last RngRun
  uint64_t m_rngRun; // current value for RngRun
  std::string m_csvFileNamePrefix; // file name for writing simulation summary results
  double m_simDuration;
};

RoutingExperiment::RoutingExperiment (uint64_t stopRun, std::string fn):
    m_startRngRun (1), 
    m_stopRngRun (stopRun),
    m_rngRun (1),
    m_csvFileNamePrefix (fn), // Default name is Net-Summary
    m_simDuration (0.0)
{
	NS_ASSERT_MSG (m_startRngRun <= m_stopRngRun, "First run number must be less or equal to last.");
}

RoutingExperiment::RoutingExperiment (uint64_t startRun, uint64_t stopRun, std::string fn):
    m_startRngRun (startRun), // default is only one simulation run
    m_stopRngRun (stopRun),
    m_rngRun (startRun),
  	m_csvFileNamePrefix (fn), // Default name is Net-Summary
    m_simDuration (0.0)
{
	NS_ASSERT_MSG (m_startRngRun <= m_stopRngRun, "First run number must be less or equal to last.");
}

void
RoutingExperiment::WriteToSummaryFile (RunSummary srs)
{
  std::ofstream out;
  if (m_rngRun == m_startRngRun)
    {
      out.open ((m_csvFileNamePrefix + "-Summary.csv").c_str (), std::ofstream::out | std::ofstream::trunc);
      out << "Rng Run, Number of Flows, Throughput [bps],, Tx Packets,, Rx Packets,, Lost Packets,, Lost Ratio [%],, PHY Tx Packets,, Useful Traffic Ratio [%],,"
          << "E2E Delay Min [ms],, E2E Delay Max [ms],, E2E Delay Average [ms],, E2E Delay Median Estimate [ms],, E2E Delay Jitter [ms],, Sim. Duration"
          << std::endl;
      out << ", , all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg"
          << "  , all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg, all flows avg, all packets avg"
          << "  , all flows avg, all packets avg, all packets avg, all packets avg, [min], [day hour min sec]"
          << std::endl;
    }
  else
    {
      out.open ((m_csvFileNamePrefix + "-Summary.csv").c_str (), std::ofstream::out | std::ofstream::app);
    }
  out << m_rngRun << "," << srs.numberOfFlows << ","
      << srs.aaf.throughput << "," << srs.aap.throughput << ","
      << srs.aaf.txPackets << "," << srs.aap.txPackets << ","
      << srs.aaf.rxPackets << "," << srs.aap.rxPackets << ","
      << srs.aaf.lostPackets << "," << srs.aap.lostPackets << ","
      << srs.aaf.lostRatio << ","<< srs.aap.lostRatio << ","
      << "," << srs.aap.phyTxPkts << ","
      << "," << srs.aap.usefullNetTraffic << ","
      << srs.aaf.e2eDelayMin * 1000.0 << "," << srs.aap.e2eDelayMin * 1000.0 << ","
      << srs.aaf.e2eDelayMax * 1000.0 << "," << srs.aap.e2eDelayMax * 1000.0 << ","
      << srs.aaf.e2eDelayAverage * 1000.0 << "," << srs.aap.e2eDelayAverage * 1000.0 << ","
      << srs.aaf.e2eDelayMedianEstimate * 1000.0 << "," << srs.aap.e2eDelayMedianEstimate * 1000.0 << ","
      << srs.aaf.e2eDelayJitter * 1000.0 << "," << srs.aap.e2eDelayJitter * 1000.0;

  int days = std::floor (m_simDuration / 86400.0);
  int hours = std::floor ((m_simDuration - days * 86400.0) / 3600.0);
  int min = std::floor ((m_simDuration - days * 86400.0 - hours * 3600.0) / 60.0);
  int sec = std::floor (m_simDuration - days * 86400.0 - hours * 3600.0 - min * 60.0);
  
  out << "," << m_simDuration / 60.0 << "," 
      << days << "d " << hours << "h " << min << "m " << sec << "s";
  out << std::endl;
  
  if (m_rngRun == m_stopRngRun)
    {
      out << std::endl;
      out << "," << "Min,"
                    << "=MIN(C3:C" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(D3:D" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(E3:E" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(F3:F" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(G3:G" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(H3:H" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(I3:I" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(J3:J" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(K3:K" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(L3:L" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MIN(M3:M" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(N3:N" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MIN(O3:O" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(P3:P" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(Q3:Q" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(R3:R" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(S3:S" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(T3:T" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(U3:U" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(V3:V" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(W3:W" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(X3:X" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(Y3:Y" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(Z3:Z" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MIN(AA3:AA" << m_stopRngRun - m_startRngRun + 3 << ")"
                    << std::endl;
      out << "," << "Max,"
                    << "=MAX(C3:C" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(D3:D" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(E3:E" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(F3:F" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(G3:G" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(H3:H" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(I3:I" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(J3:J" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(K3:K" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(L3:L" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MAX(M3:M" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(N3:N" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MAX(O3:O" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(P3:P" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(Q3:Q" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(R3:R" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(S3:S" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(T3:T" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(U3:U" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(V3:V" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(W3:W" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(X3:X" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(Y3:Y" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(Z3:Z" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MAX(AA3:AA" << m_stopRngRun - m_startRngRun + 3 << ")"
                    << std::endl;
      out << "," << "Average,"
                    << "=AVERAGE(C3:C" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(D3:D" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(E3:E" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(F3:F" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(G3:G" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(H3:H" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(I3:I" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(J3:J" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(K3:K" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(L3:L" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=AVERAGE(M3:M" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(N3:N" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=AVERAGE(O3:O" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(P3:P" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(Q3:Q" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(R3:R" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(S3:S" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(T3:T" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(U3:U" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(V3:V" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(W3:W" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(X3:X" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(Y3:Y" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(Z3:Z" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=AVERAGE(AA3:AA" << m_stopRngRun - m_startRngRun + 3 << ")"
                    << std::endl;
      out << "," << "Median,"
                    << "=MEDIAN(C3:C" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(D3:D" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(E3:E" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(F3:F" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(G3:G" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(H3:H" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(I3:I" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(J3:J" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(K3:K" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(L3:L" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MEDIAN(M3:M" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(N3:N" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << ","
//                    << "=MEDIAN(O3:O" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(P3:P" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(Q3:Q" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(R3:R" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(S3:S" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(T3:T" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(U3:U" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(V3:V" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(W3:W" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(X3:X" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(Y3:Y" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(Z3:Z" << m_stopRngRun - m_startRngRun + 3 << "),"
                    << "=MEDIAN(AA3:AA" << m_stopRngRun - m_startRngRun + 3 << ")"
                    << std::endl;
      out << "," << "Std. deviation,"
                    << "=STDEV(C3:C" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(D3:D" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(E3:E" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(F3:F" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(G3:G" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(H3:H" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(I3:I" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(J3:J" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(K3:K" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(L3:L" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << ","
//                    << "=STDEV(M3:M" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(N3:N" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << ","
//                    << "=STDEV(O3:O" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(P3:P" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(Q3:Q" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(R3:R" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(S3:S" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(T3:T" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(U3:U" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(V3:V" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(W3:W" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(X3:X" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(Y3:Y" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(Z3:Z" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << "),"
                    << "=STDEV(AA3:AA" << m_stopRngRun - m_startRngRun + 3 << ")/" << "SQRT(" << m_stopRngRun - m_startRngRun + 1 << ")"
                    << std::endl;
    }
  out.close ();
};

RunSummary
RoutingExperiment::Run (int argc, char **argv)
{
  //---------------------------------------------
  // Initial configuration and attributes
  //---------------------------------------------
  uint32_t nNodes = 100; // number of nodes
  uint32_t nSources = 10; // number of source nodes for application traffic (number of sink nodes is the same in this example)

  // SCENARIO 
  int scenario = 1; // MSBM
  // Parameters for RW mobility model
  double nodeSpeed = 15.0; // m/s
  double nodePause = 0.0; // s
  double simAreaX = 2000.0; // m
  double simAreaY = 2000.0; // m

  double simulationTime = 289.0; // in seconds
  double netStartupTime = 10.0; // [s] time before any application starts sending data

  std::string rate ("4kbps"); // application layer data rate
  std::string phyMode ("OfdmRate6MbpsBW10MHz"); // physical data rate and modulation type
  uint32_t packetSize = 512; // Bytes

  double txp = 20; // dBm, transmission power
  uint32_t lossModel = 3; ///< loss model [default: TwoRayGroundPropagationLossModel]
  bool fading = 0; // 0=None; 1=Nakagami;
  
  uint32_t routingProtocol = 2; ///< routing protocol, AODV default
  int routingTables = 0; ///< routing tables

  bool verbose = false;

  CommandLine cmd;
  cmd.AddValue ("csvFileNamePrefix", "The name prefix of the CSV output file (without .csv extension)", m_csvFileNamePrefix);
  cmd.AddValue ("nNodes", "Number of nodes in simulation", nNodes);
  cmd.AddValue ("nSources", "Number of nodes that send data (max = nNodes/2)", nSources);
  cmd.AddValue ("simTime", "Duration of one simulation run.", simulationTime);

  cmd.AddValue ("currentRngRun", "Current number of RngRun.", m_rngRun);
  cmd.AddValue ("startRngRun", "Start number of RngRun. Used in external rng run generation.", m_startRngRun);
  cmd.AddValue ("stopRngRun", "End number of RngRun (must be greater then or equal to startRngNum). Used in external rng run generation.", m_stopRngRun);

  cmd.AddValue ("dataRate", "Application data rate.", rate);
  cmd.AddValue ("packetSize", "Application test packet size.", packetSize);

  cmd.AddValue ("lossModel", "Propagation loss model: 1=Friis; 2=ItuR1411Los; 3=TwoRayGround; 4=LogDistance", lossModel);
  cmd.AddValue ("fading", "0=None;1=Nakagami;(buildings=1 overrides)", fading);
  cmd.AddValue ("txp", "Transmission power.", txp);

  cmd.AddValue ("scenario", "0=RW; 1=MSBM scenario; 2=MG-2x2mk-TrafficLight", scenario);
  cmd.AddValue ("width", "Width of simulation area (X-axis).", simAreaX);
  cmd.AddValue ("height", "Height of simulation area (Y-axis).", simAreaY);
  cmd.AddValue ("nodeSpeed", "Max node speed.", nodeSpeed);
  cmd.AddValue ("routingTables", "Dump routing tables at t=5 seconds", routingTables);
  cmd.AddValue ("routingProtocol", "Pouting protocol: 1=OLSR; 2=AODV; 3=DSDV; 4=DSR", routingProtocol);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);

  // Should be placed after cmd.Parse () because user can overload rng run number with command line option "--currentRngRun"
  RngSeedManager::SetRun (m_rngRun);

  // Disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // Turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  //---------------------------------------------
  // Creating vehicle nodes
  //---------------------------------------------
  NodeContainer vehicles;
  vehicles.Create (nNodes);

  //---------------------------------------------
  // Channel configuration
  //---------------------------------------------
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  
  std::string lossModelName;
  std::string lm;
  double freq = 5.9e9; // 802.11p 5.9 GHz
  if (lossModel == 1)
    {
      lossModelName = "ns3::FriisPropagationLossModel";
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq));
      lm = "Fri";
    }
  else if (lossModel == 2)
    {
      lossModelName = "ns3::ItuR1411LosPropagationLossModel";
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq));
      lm = "ITUR1411";
    }
  else if (lossModel == 3)
    {
      lossModelName = "ns3::TwoRayGroundPropagationLossModel";
      lm = "TRG";
      // two-ray requires antenna height (else defaults to Friss)
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq), "HeightAboveZ", DoubleValue (1.5));
    }
  else if (lossModel == 4)
    {
      lossModelName = "ns3::LogDistancePropagationLossModel";
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq));
      lm = "Log";
    }
  else
    {
      // Unsupported propagation loss model.
      NS_LOG_ERROR ("Invalid propagation loss model specified.  Values must be [1-4], where 1=Friis;2=ItuR1411Los;3=TwoRayGround;4=LogDistance");
    }
  // Propagation loss models are additive, so we can add Nakagami feding
  if (fading != 0)
    {
      // if no obstacle model, then use Nakagami fading if requested
      wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
      lm += "_Nak";
    }
  // create the channel
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();

  //---------------------------------------------
  // NIC: PHY + MAC configuration 
  //---------------------------------------------
  // Set the wifi NICs we want
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (channel);
  // ns-3 supports generate a pcap trace
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

  // Set Tx Power
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  // Add a mac and disable rate control
  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  if (verbose)
    {
      wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
    }
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode));
  NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, vehicles);

  //---------------------------------------------
  // Mobility configuration
  //---------------------------------------------
  std::string sc;
  switch (scenario){
  case 0:
  {
    sc = "RW";
    MobilityHelper vehicleMobility;
    //int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    std::stringstream ssX;
    ssX << "ns3::UniformRandomVariable[Min=0.0|Max=" << simAreaX << "]";
    std::stringstream ssY;
    ssY << "ns3::UniformRandomVariable[Min=0.0|Max=" << simAreaY << "]";
    ObjectFactory pos;
    pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue (ssX.str ()));
    pos.Set ("Y", StringValue (ssY.str ()));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
    //streamIndex += taPositionAlloc->AssignStreams (streamIndex);

    std::stringstream ssSpeed;
    //ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    //ssSpeed << "ns3::UniformRandomVariable[Min=" << 0.9*nodeSpeed << "|Max=" << 1.1*nodeSpeed << "]";
    ssSpeed << "ns3::NormalRandomVariable[Mean=" << nodeSpeed << "|Variance=" << (nodeSpeed/20)*(nodeSpeed/20) << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    vehicleMobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                    "Speed", StringValue (ssSpeed.str ()),
                                    "Pause", StringValue (ssPause.str ()),
                                    "PositionAllocator", PointerValue (taPositionAlloc));
    vehicleMobility.SetPositionAllocator (taPositionAlloc);
    vehicleMobility.Install (vehicles);
    //streamIndex += vehicleMobility.AssignStreams (vehicles, streamIndex);
    //NS_UNUSED (streamIndex); // From this point, streamIndex is unused
    break;
  }
  case 1:
  {
    sc = "MG_2x2km_semafor";
    std::string traceFile;
    switch (nNodes)
    {
    case 50:
      {
        traceFile = std::string("scratch/ns2Trace-050.txt");
        break;
      }
    case 100:
      {
        traceFile = std::string("scratch/ns2Trace-100.txt");
        break;
      }
    case 150:
      {
        traceFile = std::string("scratch/ns2Trace-150.txt");
        break;
      }
    case 200:
      {
        traceFile = std::string("scratch/ns2Trace-200.txt");
        break;
      }
    case 250:
      {
        traceFile = std::string("scratch/ns2Trace-250.txt");
        break;
      }
    case 300:
      {
        traceFile = std::string("scratch/ns2Trace-300.txt");
        break;
      }
    case 350:
      {
        traceFile = std::string("scratch/ns2Trace-350.txt");
        break;
      }
    case 400:
      {
        traceFile = std::string("scratch/ns2Trace-400.txt");
        break;
      }
    case 450:
      {
        traceFile = std::string("scratch/ns2Trace-450.txt");
        break;
      }
    case 500:
      {
        traceFile = std::string("scratch/ns2Trace-500.txt");
        break;
      }
    case 550:
      {
        traceFile = std::string("scratch/ns2Trace-550.txt");
        break;
      }
    case 600:
      {
        traceFile = std::string("scratch/ns2Trace-600.txt");
        break;
      }
    case 650:
      {
        traceFile = std::string("scratch/ns2Trace-650.txt");
        break;
      }
    case 700:
      {
        traceFile = std::string("scratch/ns2Trace-700.txt");
        break;
      }
    default:
      NS_ASSERT_MSG (0, "Number of vehicles not supported.");
    }
    Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
    ns2.Install ();
    break;
  }
  case 2:
  {
	  sc = "MG_2x2km_semafor";
    //std::string traceFile = std::string("scratch/mg-telfor-15mps-semafor-") + std::to_string(nNodes) + std::string("-fcd.txt");
    std::string traceFile = std::string("scratch/mg-telfor-15mps-semafor-350-fcd.txt");
	  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

	  // configure movements for each node, while reading
	  ns2.Install ();
	  break;
  }
  default:
	  NS_LOG_UNCOND ("Scenario not supported");
	  NS_ASSERT (0);
  }

  if (verbose){
	  for (NodeContainer::Iterator j = vehicles.Begin (); j != vehicles.End (); ++j)
		{
		  Ptr<Node> object = *j;
		  Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		  NS_ASSERT (position != NULL);
		  Vector pos = position->GetPosition ();
		  NS_LOG_UNCOND ("Node: " << object->GetId() << " x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z );
		}
  }

  //---------------------------------------------
  // Routing and Internet configuration
  //---------------------------------------------
  AodvHelper aodv;
  //aodv.Set ("EnableHello", BooleanValue (false)); // disable hello packets to prevent large overheads
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  Time rtt = Time (5.0);
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rtw = ascii.CreateFileStream ("routing_table");
  std::string rp; ///< protocol name
  switch (routingProtocol)
    {
    case 0:
      rp = "NONE";
      break;
    case 1:
      if (routingTables != 0)
        olsr.PrintRoutingTableAllAt (rtt, rtw);
      list.Add (olsr, 100);
      rp = "OLSR";
      break;
    case 2:
      if (routingTables != 0)
        aodv.PrintRoutingTableAllAt (rtt, rtw);
      list.Add (aodv, 100);
      rp = "AODV";
      break;
    case 3:
      if (routingTables != 0) 
        dsdv.PrintRoutingTableAllAt (rtt, rtw);
      list.Add (dsdv, 100);
      rp = "DSDV";
      break;
    case 4:
      rp = "DSR";
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << routingProtocol);
      break;
    }
  if (routingProtocol == 4)
    {
      internet.Install (vehicles);
      dsrMain.Install (dsr, vehicles);
    }
  else
   {
      internet.SetRoutingHelper (list);
      internet.Install (vehicles);
    }

  //Assigning ip address
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (devices);

  //---------------------------------------------
  // Applications configuration
  //---------------------------------------------
  std::string tp = "UDP";
  std::string transportProtocolFactory = "ns3::UdpSocketFactory"; // protocol for transport layer
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0));
  x->SetAttribute ("Max", DoubleValue (nNodes-1));
  std::vector<int> ss; // sources and sinks
  uint32_t port = 80;
  int p, q;
  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
  for (uint32_t i = 0; i<nSources; i++)
    {
      std::ostringstream oss;
      while (1) // choose random source that is unique (node that is not used before as source or sink)
        {
          p = (int) x->GetInteger ();
          std::vector<int>::iterator it;
          it = find(ss.begin(), ss.end(), p);
          if (it == ss.end())
            { ss.push_back(p); break; }
        }
      while (1) // choose random sink that is unique (node that is not used before as source or sink)
        {
          q = (int) x->GetInteger ();
          std::vector<int>::iterator it;
          it = find(ss.begin(), ss.end(), q);
          if (it == ss.end())
            { ss.push_back(q); NS_LOG_UNCOND(p << " -> " << q); break; }
        }
      oss <<  "10.1.0." << q+1; //destination address
      InetSocketAddress destinationAddress = InetSocketAddress (Ipv4Address (oss.str().c_str ()), port); // destination address for sorce apps
      InetSocketAddress sinkReceivingAddress = InetSocketAddress (Ipv4Address::GetAny (), port); // sink nodes receive from any address
      double appJitter = var->GetValue (0.0,0.5); // half of a second jitter
    
      // Source
      StatsSourceHelper sourceAppH (transportProtocolFactory, destinationAddress);
      sourceAppH.SetConstantRate (DataRate (rate));
      sourceAppH.SetAttribute ("PacketSize", UintegerValue(packetSize));
      ApplicationContainer sourceApps = sourceAppH.Install (vehicles.Get (p));
      sourceApps.Start (Seconds (netStartupTime+appJitter));
      sourceApps.Stop (Seconds (netStartupTime+simulationTime+appJitter)); // Every app stops after finishes runnig of "simulationTime" seconds
    
      // Sink 
      StatsSinkHelper sink (transportProtocolFactory, sinkReceivingAddress);
      ApplicationContainer sinkApps = sink.Install (vehicles.Get (q));
      sinkApps.Start (Seconds (0.0)); // start at the begining and wait for first packet
      sinkApps.Stop (Seconds (netStartupTime+simulationTime)); // stop a bit later then source to receive the last packet
    }
 
  //---------------------------------------------
  // Tracing configuration
  //---------------------------------------------

  /* // PCAP tracing
  wifiPhy.EnablePcap ("wave-simple-80211p", devices); */

  /* // NetAnim
  AnimationInterface anim (m_csvFileNamePrefix + "-run_" + std::to_string (m_rngRun) + ".xml");
  anim.SetMaxPktsPerTraceFile(500000);
  anim.EnablePacketMetadata (true);  */

  /* // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();  */

  // NPAF configuration
  // File name
  m_csvFileNamePrefix += "-Sc_" + sc + "-Loss_" + lm + "-Rout_" + rp + "-Tr_" + tp + "-" + std::to_string (nSources) + "of" + std::to_string (nNodes)
                      + "-" + rate + "-" + std::to_string (packetSize) + "B";
  StatsFlows oneRunStats (m_rngRun, m_csvFileNamePrefix, false, false); // current RngRun, file name, RunSummary to file, EveryPacket to file
  //StatsFlows oneRunStats (m_rngRun, m_csvFileNamePrefix); // current RngRun, file name, false, false
  //oneRunStats.SetHistResolution (0.0001); // sets resolution in seconds
  //sf.EnableWriteEvryRunSummary (); or sf.DisableWriteEvryRunSummary (); -> file: <m_csvFileNamePrefix>-Run<RngRun>.csv
  //sf.DisableWriteEveryPacket ();   or sf.EnableWriteEveryPacket ();    -> file: <m_csvFileNamePrefix>-Run<RngRun>.csv
  
  //---------------------------------------------
  // Running one simulation
  //---------------------------------------------
  Simulator::Stop (Seconds (netStartupTime+simulationTime+1));
  Simulator::Schedule (Seconds (0), &PrintCurrentTime);
  Simulator::Run ();
  RunSummary srs = oneRunStats.Finalize (); // Write final statistics to file and return run summary
  Simulator::Destroy (); // End of simulation
  return srs;
}


//////////////////////////////////////////////
// main function
////////////////////////////////////////////// 
int
main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  auto start = std::chrono::system_clock::now();
  RunSummary srs = experiment.Run (argc,argv);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  experiment.SetSimDuration (elapsed_seconds.count());
  experiment.WriteToSummaryFile (srs); // -> file: <m_csvFileNamePrefix>-Summary.csv
  return 0;
}




