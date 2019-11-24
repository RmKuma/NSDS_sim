/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.
#include <iostream>
#include <ctime>
#include <string>
#include <fstream>
#include <cstring>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/nvmeof-module.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NVMeoFTCPExample");

int
main (int argc, char *argv[])
{

  bool tracing = false;
  uint32_t maxBytes = 0;

//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.Parse (argc, argv);
 

  // ============ MQSim Configuration =============
  int MQSim_argc = 5;
  const char* MQSim_argv[5];
  std::string strs[5] = {"./MQSim", "-i", "src/nvmeof/helper/src/ssdconfig.xml", "-w", "src/nvmeof/helper/src/workload.xml"};
  for(int i =0; i< 5; i++){
    MQSim_argv[i] = strs[i].c_str();
  }

  string ssd_config_file_path, workload_defs_file_path;
 
  command_line_args(MQSim_argv, ssd_config_file_path, workload_defs_file_path);

  Execution_Parameter_Set* exec_params = new Execution_Parameter_Set;
  read_configuration_parameters(ssd_config_file_path, exec_params);
  std::vector<std::vector<IO_Flow_Parameter_Set*>*>* io_scenarios = read_workload_definitions(workload_defs_file_path);
  // ============ MQSim Configuration =============
 
 
  int cntr = 1;
  for(auto io_scen = io_scenarios->begin(); io_scen != io_scenarios->end(); io_scen++, cntr++){

	// ============ MQSim IO Flow Definition ===========
	exec_params->Host_Configuration.IO_Flow_Definitions.clear();
	for (auto io_flow_def = (*io_scen)->begin(); io_flow_def != (*io_scen)->end(); io_flow_def++) {
	  exec_params->Host_Configuration.IO_Flow_Definitions.push_back(*io_flow_def);
	}	
	
	MQSimulator->Reset();

	SSD_Device ssd_device (&exec_params->SSD_Device_Configuration, &exec_params->Host_Configuration.IO_Flow_Definitions);
	exec_params->Host_Configuration.Input_file_path = workload_defs_file_path.substr(0, workload_defs_file_path.find_last_of("."));
	Host_System hostsys (&exec_params->Host_Configuration, exec_params->SSD_Device_Configuration.Enabled_Preconditioning, ssd_device.Host_interface);
	hostsys.Attach_ssd_device(&ssd_device);

	// ============ MQSim IO Flow Definition ===========
	//
	
	NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));


    NetDeviceContainer devices;
    devices = pointToPoint.Install (nodes);

//
// Install the internet stack on the nodes
//
    InternetStackHelper internet;
    internet.Install (nodes);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign (devices);

    NS_LOG_INFO ("Create Applications.");

    uint16_t port = 10000;  // well-known echo port number

    NVMeoFHostHelper host ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    host.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    ApplicationContainer hostApps = host.Install (nodes.Get (0));
    hostApps.Start (MicroSeconds (0.0));
    hostApps.Stop (MicroSeconds (1000000000.0));

	

//
// Create a PacketSinkApplication and install it on node 1
//
    NVMeoFTargetHelper target ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer targetApps = target.Install (nodes.Get (1));
    targetApps.Start (MicroSeconds (0.0));
    targetApps.Stop (MicroSeconds (1000000000.0));

//
// Now, do the actual simulation.
// 
    MQSimulator->Start_simulation();
	
	std::cout << "NS Start" << std::endl;

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (MicroSeconds (1000000000.0));
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");

	collect_results(ssd_device, hostsys, ("result_scenario_" + std::to_string(cntr) + ".xml").c_str());

    Ptr<NVMeoFHostApplication> host1 = DynamicCast<NVMeoFHostApplication> (hostApps.Get (0));
    Ptr<NVMeoFTargetApplication> target1 = DynamicCast<NVMeoFTargetApplication> (targetApps.Get (0));
    std::cout << "Host Total Bytes Received: " << host1->GetTotalRx () << std::endl;
    std::cout << "Target Total Bytes Received: " << target1->GetTotalRx () << std::endl;
  
  }
}
