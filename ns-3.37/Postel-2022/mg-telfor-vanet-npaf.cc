/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Belgrade
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nenad Jevtic (n.jevtic@sf.bg.ac.rs), Marija Malnar (m.malnar@sf.bg.ac.rs)
 */

/* 
  NOVO U VERZIJAMA:

  - 2.2.1 ...
  - 2.3   U odnosu na prethodnu verziju je dodat novi scenario mobilnosti (scenario 4) koje je MG model generisan iz Bonn Motion alata sa srednjom brzinom od 17m/s.
*/


#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>    

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"

#include "ns3/aodv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"

#include "ns3/npaf-module.h"
#include "ns3/netanim-module.h"

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

#include "ns3/random-variable-stream.h"
#include <vector>

using namespace ns3;
using namespace npaf;

NS_LOG_COMPONENT_DEFINE ("wirelessnetworks");


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
// controls one program execution (run)
// holds data from current run
/////////////////////////////////////////////
class RoutingExperiment
{
public:
  RoutingExperiment (uint64_t stopRun = 1, std::string fn = "Net"); // default is only one simulation run
  RoutingExperiment (uint64_t startRun, uint64_t stopRun, std::string fn = "Net");
  RunSummary Run (int argc, char **argv);
  void WriteToSummaryFile (RunSummary srs);

  void SetRngRun (uint64_t run) { m_rngRun = run; };
  uint64_t GetRngRun () { return m_rngRun; };
  void SetStartRngRun (uint64_t run) { m_startRngRun = run; };
  uint64_t GetStartRngRun () { return m_startRngRun; };
  void SetStopRngRun (uint64_t run) { m_stopRngRun = run; };
  uint64_t GetStopRngRun () { return m_stopRngRun; };
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
	m_csvFileNamePrefix (fn) // Default name is Net-Summary
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

  //  Packet::EnablePrinting ();

  uint32_t nNodes = 200; // number of nodes
  uint32_t nSources = 20; // number of source nodes for application traffic (number of sink nodes is the same in this example)

  // Parameters for RW mobility model
  double nodeSpeed = 12.0; // m/s
  double nodePause = 0.0; // s
  double simAreaX = 2000.0; // m
  double simAreaY = 2000.0; // m

  double simulationDuration = 500.0; // in seconds
  double netStartupTime = 100.0; // [s] time before any application starts sending data

  std::string rate ("4096bps"); // application layer data rate

  uint32_t transportProtocol = 1; ///< transport protocol, UTP default

  uint32_t packetSize = 512; // Bytes

  uint32_t port = 80;

  double txp = 20; // dBm, transmission power
  std::string phyMode ("OfdmRate6MbpsBW10MHz"); // physical data rate and modulation type
  uint32_t lossModel = 3; ///< loss model [default: TwoRayGroundPropagationLossModel]
  bool fading = 0; // 0=None; 1=Nakagami;
  
  bool verbose = false;

  int scenario = 1; // ManhattanGrid with semaphore

  uint32_t routingProtocol = 2; ///< routing protocol, AODV default
  std::string routingProtocolName = ""; // name not specified
  int routingTables = 0; ///< routing tables

  CommandLine cmd;
  cmd.AddValue ("csvFileNamePrefix", "The name prefix of the CSV output file (without .csv extension)", m_csvFileNamePrefix);
  cmd.AddValue ("nNodes", "Number of nodes in simulation", nNodes);
  cmd.AddValue ("nSources", "Number of nodes that send data (max = nNodes/2)", nSources);
  cmd.AddValue ("currentRngRun", "Current number of RngRun if external rng run control is used. It must be used with --externalRngRunControl=1 to prevent authomatic rng run control. Also, must be between startRngRun and stopRngRun. Otherwise can produce unpredictable result.", m_rngRun);
  cmd.AddValue ("startRngRun", "Start number of RngRun. Used in both internal and external rng run generation.", m_startRngRun);
  cmd.AddValue ("stopRngRun", "End number of RngRun (must be greater then or equal to startRngNum). Used in both internal and external rng run generation.", m_stopRngRun);
  cmd.AddValue ("simTime", "Duration of one simulation run.", simulationDuration);
  cmd.AddValue ("dataRate", "Application data rate.", rate);
  cmd.AddValue ("packetSize", "Application test packet size.", packetSize);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("lossModel", "Propagation loss model: 1=Friis; 2=ItuR1411Los; 3=TwoRayGround; 4=LogDistance", lossModel);
  cmd.AddValue ("fading", "0=None;1=Nakagami;(buildings=1 overrides)", fading);
  cmd.AddValue ("scenario", "0=RW;1=MG-2x2mk-TrafficLight; 2=MG-2x2km-; 3=MG-1x1km-TrafficLight", scenario);
  cmd.AddValue ("routingTables", "Dump routing tables at t=5 seconds", routingTables);
  cmd.AddValue ("routingProtocol", "Pouting protocol: 1=OLSR; 2=AODV; 3=DSDV; 4=DSR", routingProtocol);
  cmd.AddValue ("routingProtocolName", "Name of the routing protocol used for creating file name for storing results, if not supplied default name is used", routingProtocolName);
  cmd.AddValue ("transportProtocol", "Transport protocol: 1=UTP; 2=TCP", transportProtocol);
  cmd.AddValue ("width", "Width of simulation area (X-axis).", simAreaX);
  cmd.AddValue ("height", "Height of simulation area (Y-axis).", simAreaY);
  cmd.AddValue ("nodeSpeed", "Max node speed.", nodeSpeed);
  cmd.AddValue ("txp", "Transmission power.", txp);
  
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
  if (lossModel == 1)
    {
      lossModelName = "ns3::FriisPropagationLossModel";
      lm = "Fri";
    }
  else if (lossModel == 2)
    {
      lossModelName = "ns3::ItuR1411LosPropagationLossModel";
      lm = "ITUR1411";
    }
  else if (lossModel == 3)
    {
      lossModelName = "ns3::TwoRayGroundPropagationLossModel";
      lm = "TRG";
    }
  else if (lossModel == 4)
    {
      lossModelName = "ns3::LogDistancePropagationLossModel";
      lm = "Log";
    }
  else
    {
      // Unsupported propagation loss model.
      // Treating as ERROR
      NS_LOG_ERROR ("Invalid propagation loss model specified.  Values must be [1-4], where 1=Friis;2=ItuR1411Los;3=TwoRayGround;4=LogDistance");
    }
  double freq = 5.9e9; // 802.11p 5.9 GHz
  if (lossModel == 3)
    {
      // two-ray requires antenna height (else defaults to Friss)
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq), "HeightAboveZ", DoubleValue (1.5));
    }
  else
    {
      wifiChannel.AddPropagationLoss (lossModelName, "Frequency", DoubleValue (freq));
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

  // Tracing
  //wifiPhy.EnablePcap ("wave-simple-80211p", devices);


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
    //std::string traceFile = std::string("scratch/mg-telfor-15mps-semafor-") + std::to_string(nNodes) + std::string("-fcd.txt");
    std::string traceFile = std::string("scratch/mg-telfor-15mps-semafor-350-fcd.txt");
	  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

	  // configure movements for each node, while reading
	  ns2.Install ();
	  break;
  }
  case 2:
  {
    sc = "MG_2x2km";
    //std::string traceFile = std::string("scratch/mg-telfor-15mps-") + std::to_string(nNodes) + std::string("-fcd.txt");
    std::string traceFile = std::string("scratch/mg-telfor-15mps-350-fcd.txt");
    Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

    // configure movements for each node, while reading
    ns2.Install ();
    break;
  }
  case 3:
  {
    sc = "MG_1x1km_semafor";
    //std::string traceFile = std::string("scratch/mg-telfor-15mps-") + std::to_string(nNodes) + std::string("-fcd.txt");
    std::string traceFile = std::string("scratch/mg-telfor-13mps-semafor-250-fcd.txt");
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
	  // iterate our nodes and print their position.
	  for (NodeContainer::Iterator j = vehicles.Begin ();
		   j != vehicles.End (); ++j)
		{
		  Ptr<Node> object = *j;
		  Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		  NS_ASSERT (position != 0);
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
        {
          olsr.PrintRoutingTableAllAt (rtt, rtw);
        }
      list.Add (olsr, 100);
      if (routingProtocolName == "")
        {
          rp = "OLSR";
        }
      else
        {
          rp = routingProtocolName;
        }
      break;
    case 2:
      if (routingTables != 0)
        {
          aodv.PrintRoutingTableAllAt (rtt, rtw);
        }
      list.Add (aodv, 100);
      if (routingProtocolName == "")
        {
          rp = "AODV";
        }
      else
        {
          rp = routingProtocolName;
        }
      break;
    case 3:
      if (routingTables != 0)
        {
          dsdv.PrintRoutingTableAllAt (rtt, rtw);
        }
      list.Add (dsdv, 100);
      if (routingProtocolName == "")
        {
          rp = "DSDV";
        }
      else
        {
          rp = routingProtocolName;
        }
      break;
    case 4:
      // setup is later
      if (routingProtocolName == "")
        {
          rp = "DSR";
        }
      else
        {
          rp = routingProtocolName;
        }
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << routingProtocol);
      break;
    }

  if (routingProtocol < 4)
    {
      internet.SetRoutingHelper (list);
      internet.Install (vehicles);
    }
  else if (routingProtocol == 4)
    {
      internet.Install (vehicles);
      dsrMain.Install (dsr, vehicles);
    }
    
  //Assigning ip address
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (devices);

  //---------------------------------------------
  // Applications configuration
  //---------------------------------------------
  std::string tp;
  std::string transportProtocolFactory;
  if (transportProtocol == 1)
    {
      transportProtocolFactory = "ns3::UdpSocketFactory"; // protocol for transport layer
      tp = "UDP";
    }
  else
    {
      transportProtocolFactory = "ns3::TcpSocketFactory"; // protocol for transport layer
      tp = "TCP";
    }

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (0));
  x->SetAttribute ("Max", DoubleValue (nNodes-1));
  std::vector<int> ss; // sources and sinks
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
      sourceApps.Stop (Seconds (netStartupTime+simulationDuration+appJitter)); // Every app stops after finishes runnig of "simulationDuration" seconds
    
      // Sink 
      StatsSinkHelper sink (transportProtocolFactory, sinkReceivingAddress);
      ApplicationContainer sinkApps = sink.Install (vehicles.Get (q));
      sinkApps.Start (Seconds (0.0)); // start at the begining and wait for first packet
      sinkApps.Stop (Seconds (netStartupTime+simulationDuration+1)); // stop a bit later then source to receive the last packet
    }
 
  //---------------------------------------------
  // Tracing configuration
  //---------------------------------------------
  // File name
  if (m_csvFileNamePrefix == "Net")
    {
	    m_csvFileNamePrefix += "-Sc_" + sc 
	                    + "-Loss_" + lm 
	                    + "-Rout_" + rp
	                    + "-Tr_" + tp
                        + "-" + std::to_string (nSources) + "of" + std::to_string (nNodes)
                        + "-" + rate
                        + "-" + std::to_string (packetSize) + "B";
    }
  
  StatsFlows oneRunStats (m_rngRun, m_csvFileNamePrefix, false, false); // current RngRun, file name, RunSummary to file, EveryPacket to file
  //StatsFlows oneRunStats (m_rngRun, m_csvFileNamePrefix); // current RngRun, file name, false, false
  oneRunStats.SetHistResolution (0.0001); // sets resolution in seconds
  //sf.EnableWriteEvryRunSummary (); or sf.DisableWriteEvryRunSummary (); -> file: <m_csvFileNamePrefix>-Run<RngRun>.csv
  //sf.DisableWriteEveryPacket ();   or sf.EnableWriteEveryPacket ();    -> file: <m_csvFileNamePrefix>-Run<RngRun>.csv
  
  /*AnimationInterface anim (m_csvFileNamePrefix + "-run_" + std::to_string (m_rngRun) + ".xml");
  anim.SetMaxPktsPerTraceFile(500000);
  anim.EnablePacketMetadata (true);*/
  
  //---------------------------------------------
  // Running one simulation
  //---------------------------------------------
  // Start-stop simulation
  // Stop event is set so that all applications have enough tie to finish 
  Simulator::Stop (Seconds (netStartupTime+simulationDuration+1));
  NS_LOG_INFO ("Current simulation run [" << m_startRngRun << "->" << m_stopRngRun << "]: " << m_rngRun);
  Simulator::Schedule (Seconds (0), &PrintCurrentTime);
  Simulator::Run ();
  // Write final statistics to file and return run summary
  RunSummary srs = oneRunStats.Finalize ();

  // End of simulation
  Simulator::Destroy ();
  return srs;
}


//////////////////////////////////////////////
// main function
// controls multiple simulation execution (multiple runs)
// holds data between runs 
////////////////////////////////////////////// 
int
main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  
  // Run the experiment
  // Also parse command line arguments if any (this includes rng run parameters)
  auto start = std::chrono::system_clock::now();
  RunSummary srs = experiment.Run (argc,argv);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  experiment.SetSimDuration (elapsed_seconds.count());
  experiment.WriteToSummaryFile (srs); // -> file: <m_csvFileNamePrefix>-Summary.csv

  return 0;
}




