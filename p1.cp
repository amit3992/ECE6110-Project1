// Amit Kulkarni
// GT ID: 903038158
#include <iostream>
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h" 
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P1");

int
main (int argc, char *argv[])
{

  Time::SetResolution (Time::NS);

  // Set default values of variables.
  uint nLeftLeaf = 1;
  uint nRightLeaf = 1; 
  std::string animFile = "P1.xml" ; // Name of file for animation output
  uint32_t maxBytes = 0; // value attricuted to infinity
  uint nFlows = 1; // initial value of flows
  uint32_t segSize = 512; // initial value of segment size
  uint32_t queueSize = 8000; // initial value of queue size
  uint32_t windowSize = 8000; // initial value of window size

  
// To change the value during run-time  
  CommandLine cmd;
  cmd.AddValue ("nFlows","Number of flows", nFlows);
  cmd.AddValue ("segSize","Size of each segment", segSize);
  cmd.AddValue ("queueSize","Size of queue", queueSize);
  cmd.AddValue ("windowSize","Size of window", windowSize);
  cmd.Parse (argc, argv);

  double start[nFlows];
  double elapsed[nFlows];
  double recv[nFlows];  
  double op[nFlows];
  
  if(nFlows > 0)
  {
    nLeftLeaf = nFlows;
    nRightLeaf = nFlows;
  }

  NS_LOG_INFO ("Create channels.");
  // Set values of dumb-bell leaf links.
  PointToPointHelper p2pLeaf;
  p2pLeaf.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2pLeaf.SetChannelAttribute ("Delay", StringValue ("10ms"));

  // set values of router link.
  PointToPointHelper p2pRouter;
  p2pRouter.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2pRouter.SetChannelAttribute ("Delay", StringValue ("20ms"));
  p2pRouter.SetQueue("ns3::DropTailQueue","MaxBytes",UintegerValue(queueSize));
  p2pRouter.SetQueue("ns3::DropTailQueue","Mode",StringValue("QUEUE_MODE_BYTES")); //--> check

  //  Helper function to create a dumb-bell topology
  PointToPointDumbbellHelper db (nLeftLeaf, p2pLeaf, nRightLeaf, p2pLeaf, p2pRouter);

  // Setting TCP characteristics.
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpTahoe"));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(segSize));
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(windowSize));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", StringValue("false"));

  // Install internet stack
  InternetStackHelper internet;
  db.InstallStack (internet);
 
  // Assign IP address
  // AssignIPv4Address assigns IP address to the three interfaces.
  NS_LOG_INFO ("Assign IP Addresses.");
  db.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  // Set up the acutal simulation
  Ipv4GlobalRoutingHelper::PopulateRoutingTables (); 

  RngSeedManager::SetSeed(11223344);
  Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable> ();
  rv->SetAttribute ("Stream", IntegerValue (6110));
  rv->SetAttribute ("Min", DoubleValue (0.0));
  rv->SetAttribute ("Max", DoubleValue (0.1));

  for (uint i = 0; i < nFlows; i++)
{
      start[i] = rv->GetValue();
}
  

// =================================== TOPOLOGY CREATED ===============================================


ApplicationContainer sourceApp[nFlows];
ApplicationContainer sinkApps[nFlows];


  for (uint i = 0; i < nFlows; i++)
{
    BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (db.GetRightIpv4Address(i), 9));
    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    sourceApp[i].Add(source.Install (db.GetLeft(i)));
    sourceApp[i].Start (Seconds (start[i]));
    sourceApp[i].Stop (Seconds (10.0));
 }


for (uint i = 0; i < nFlows; i++)
{

	PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(),9));
	AddressValue addr (InetSocketAddress(db.GetRightIpv4Address(i), 9));
    sink.SetAttribute("Local", addr);
    sinkApps[i].Add(sink.Install(db.GetRight(i)));
    sinkApps[i].Start (Seconds (start[i]));
    sinkApps[i].Stop (Seconds (10.0));
	
}
    
 
   
// Set the bounding box for animation
db.BoundingBox (1, 1, 100, 100);

// Create the animation object and configure for specified output
AnimationInterface anim (animFile);

Simulator::Stop (Seconds (10.0));
Simulator::Run ();
//std::cout << "Animation Trace file created:" << animFile.c_str ()<< std::endl;


for(uint i = 0; i < nFlows; i++ )
{ 
  elapsed[i] = 10.0-start[i]; 
}

for (uint x = 0; x < nFlows; x++)
{
  
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps[x].Get (0));
  recv[x] = sink1->GetTotalRx ();
  op[x] = recv[x]/elapsed[x];
  std::cout <<"flow " <<x<<" windowSize "<<windowSize<<" queueSize "<<queueSize<<" segSize "<<segSize<<" goodput "<<op[x]<<std::endl;
 
}

Simulator::Destroy ();

return 0;

}
