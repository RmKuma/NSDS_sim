/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "nvmeof-target.h"
#include "ns3/PCIe_Link.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NVMeoFTargetApplication");

NS_OBJECT_ENSURE_REGISTERED (NVMeoFTargetApplication);

TypeId 
NVMeoFTargetApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NVMeoFTargetApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<NVMeoFTargetApplication> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&NVMeoFTargetApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&NVMeoFTargetApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&NVMeoFTargetApplication::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&NVMeoFTargetApplication::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

NVMeoFTargetApplication::NVMeoFTargetApplication ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  mm_socket = 0;
  m_totalRx = 0;
}

NVMeoFTargetApplication::~NVMeoFTargetApplication()
{
  NS_LOG_FUNCTION (this);
}

void NVMeoFTargetApplication::sendRequestCallback(requestCallback rc){
	m_requestcallback = rc;
}

uint64_t NVMeoFTargetApplication::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
NVMeoFTargetApplication::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
NVMeoFTargetApplication::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void NVMeoFTargetApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void NVMeoFTargetApplication::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (m_socket->Bind (m_local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Listen ();
/*      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
*/
    }

  m_socket->SetRecvCallback (MakeCallback (&NVMeoFTargetApplication::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&NVMeoFTargetApplication::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&NVMeoFTargetApplication::HandlePeerClose, this),
    MakeCallback (&NVMeoFTargetApplication::HandlePeerError, this));  

  

}

void NVMeoFTargetApplication::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void NVMeoFTargetApplication::HandleRead (Ptr<Socket> socket)
{
  mm_socket = socket;
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      socket->GetSockName (localAddress);
	
	  // Send echo packet
      m_requestcallback ( );	  	  

      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);
    }
}


void NVMeoFTargetApplication::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void NVMeoFTargetApplication::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void NVMeoFTargetApplication::SendResult(uint64_t time)
{
	Simulator::Schedule(NanoSeconds(time), &NVMeoFTargetApplication::SendResultPacket, this);
}

void NVMeoFTargetApplication::SendResultPacket()
{
	uint64_t packetSize = 4;
	Ptr<Packet> packet = Create<Packet> (packetSize);
	mm_socket->Send(packet);
}


void NVMeoFTargetApplication::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&NVMeoFTargetApplication::HandleRead, this));
  m_socketList.push_back (s);
}

} // Namespace ns3
