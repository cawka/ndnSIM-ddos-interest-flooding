/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "token-bucket-with-per-interface-fairness.h"

#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/double.h"
#include "ns3/string.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#include <ns3/ndn-limits.h>

NS_LOG_COMPONENT_DEFINE ("ndn.fw.TokenBucketWithPerInterfaceFairness");

namespace ns3 {
namespace ndn {
namespace fw {

template<class Parent>
TypeId
TokenBucketWithPerInterfaceFairness<Parent>::GetTypeId ()
{
  static TypeId tid = TypeId ((super::GetTypeId ().GetName ()+"::TokenBucketWithPerInterfaceFairness").c_str ())
    .SetGroupName ("Ndn")
    .template SetParent<super> ()
    .template AddConstructor< TokenBucketWithPerInterfaceFairness<Parent> > ()

    .template AddAttribute ("Limit", "Limit type to be used (e.g., ns3::ndn::Limits::Window or ns3::ndn::Limits::Rate)",
                            StringValue ("ns3::ndn::Limits::Window"),
                            MakeStringAccessor (&TokenBucketWithPerInterfaceFairness<Parent>::m_limitType),
                            MakeStringChecker ())    
    ;
  
  return tid;
}

template<class Parent>
std::string
TokenBucketWithPerInterfaceFairness<Parent>::GetLogName ()
{
  return super::GetLogName ()+".TokenBucketWithPerInterfaceFairness";
}


template<class Parent>
void
TokenBucketWithPerInterfaceFairness<Parent>::AddFace (Ptr<Face> face)
{
  ObjectFactory factory (m_limitType);
  Ptr<Limits> limits = factory.template Create<Limits> ();
  limits->RegisterAvailableSlotCallback (MakeCallback (&TokenBucketWithPerInterfaceFairness<Parent>::ProcessFromQueue, this));
  face->AggregateObject (limits);
  
  super::AddFace (face);
}

template<class Parent>
void
TokenBucketWithPerInterfaceFairness<Parent>::RemoveFace (Ptr<Face> face)
{  
  for (PitQueueMap::iterator item = m_pitQueues.begin ();
       item != m_pitQueues.end ();
       item ++)
    {
      item->second.Remove (face);
    }
  m_pitQueues.erase (face);

  super::RemoveFace (face);
}

template<class Parent>
bool
TokenBucketWithPerInterfaceFairness<Parent>::TrySendOutInterest (Ptr<Face> inFace,
                                          Ptr<Face> outFace,
                                          Ptr<const Interest> interest,
                                          Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());
  // totally override all (if any) parent processing

  if (pitEntry->GetFwTag<PitQueueTag> () != boost::shared_ptr<PitQueueTag> ())
    {
      // how is it possible?  probably for the similar interests or retransmissions
      pitEntry->UpdateLifetime (Seconds (0.10));
      NS_LOG_DEBUG ("Packet is still in queue and is waiting for its processing");
      return true; // already in the queue
    }
  
  if (interest->GetInterestLifetime () < Seconds (0.1))
    {
      NS_LOG_DEBUG( "Interest lifetime is so short? [" << interest->GetInterestLifetime ().ToDouble (Time::S) << "s]");
    }
  
  pit::Entry::out_iterator outgoing =
    pitEntry->GetOutgoing ().find (outFace);

  if (outgoing != pitEntry->GetOutgoing ().end ())
    {
      // just suppress without any other action
      return false;
    }

  // NS_LOG_DEBUG ("Limit: " << outFace->GetObject<LimitsWindow> ()->GetCurrentLimit () <<
  //               ", outstanding: " << outFace->GetObject<LimitsWindow> ()->GetOutstanding ());

  Ptr<Limits> faceLimits = outFace->GetObject<Limits> ();
  if (faceLimits->IsBelowLimit ())
    {
      faceLimits->BorrowLimit ();
      pitEntry->AddOutgoing (outFace);

      //transmission
      outFace->SendInterest (interest);

      this->DidSendOutInterest (inFace, outFace, interest, pitEntry);      
      return true;
    }
  else
    {
      NS_LOG_DEBUG ("Face limit for " << interest->GetName ());
    }

  // hack
  // offset lifetime, so we don't keep entries in queue for too long
  // pitEntry->OffsetLifetime (Seconds (0.010) + );
  // std::cerr << (pitEntry->GetExpireTime () - Simulator::Now ()).ToDouble (Time::S) * 1000 << "ms" << std::endl;
  pitEntry->OffsetLifetime (Seconds (-pitEntry->GetInterest ()->GetInterestLifetime ().ToDouble (Time::S)));
  pitEntry->UpdateLifetime (Seconds (0.10));

  bool enqueued = m_pitQueues[outFace].Enqueue (inFace, pitEntry, 1.0);

  if (enqueued)
    {
      NS_LOG_DEBUG ("PIT entry is enqueued for delayed processing. Telling that we forwarding possible");
      return true;
    }
  else
    return false;
}

template<class Parent>
void
TokenBucketWithPerInterfaceFairness<Parent>::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());

  if (!PitQueue::Remove (pitEntry))
    {
      // allow bad stats only for "legitimately" timed out interests

      // consider non forwarded interests as neutral
      super::WillEraseTimedOutPendingInterest (pitEntry);
    }
  
  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      face->m_face->GetObject<Limits> ()->ReturnLimit ();
    }
}


template<class Parent>
void
TokenBucketWithPerInterfaceFairness<Parent>::WillSatisfyPendingInterest (Ptr<Face> inFace,
                                                                         Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());
  super::WillSatisfyPendingInterest (inFace, pitEntry);
  
  PitQueue::Remove (pitEntry);

  for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
       face != pitEntry->GetOutgoing ().end ();
       face ++)
    {
      face->m_face->GetObject<Limits> ()->ReturnLimit ();
    }
}


template<class Parent>
void
TokenBucketWithPerInterfaceFairness<Parent>::ProcessFromQueue ()
{
  NS_LOG_FUNCTION (this);
  
  for (PitQueueMap::iterator queue = m_pitQueues.begin ();
       queue != m_pitQueues.end ();
       queue++)
    {
      Ptr<Face> outFace = queue->first;

      NS_LOG_DEBUG ("Processing " << *outFace);

      Ptr<Limits> faceLimits = outFace->GetObject<Limits> ();
      while (!queue->second.IsEmpty () && faceLimits->IsBelowLimit ())
        {
          // now we have enqueued packet and have slot available. Send out delayed packet
          Ptr<pit::Entry> pitEntry = queue->second.Pop ();
          if (pitEntry == 0)
            {
              NS_LOG_DEBUG ("Though there are Interests in queue, weighted round robin decided that packet is not allowed yet");
              break;
            }

          // hack
          // offset lifetime back, so PIT entry wouldn't prematurely expire
          
          // std::cerr << Simulator::GetContext () << ", Lifetime before " << (pitEntry->GetExpireTime () - Simulator::Now ()).ToDouble (Time::S) << "s" << std::endl;
          pitEntry->OffsetLifetime (Seconds (-0.10) + Seconds (pitEntry->GetInterest ()->GetInterestLifetime ().ToDouble (Time::S)));
          // std::cerr << Simulator::GetContext () << ", Lifetime after " << (pitEntry->GetExpireTime () - Simulator::Now ()).ToDouble (Time::S) << "s" << std::endl;

          faceLimits->BorrowLimit ();
          pitEntry->AddOutgoing (outFace);

          outFace->SendInterest (pitEntry->GetInterest ());
          this->DidSendOutInterest (queue->first, outFace, pitEntry->GetInterest (), pitEntry);
        }
    }
}

} // namespace fw
} // namespace ndn
} // namespace ns3


#include <ns3/ndnSIM/model/fw/best-route.h>
#include "satisfaction-based-pushback.h"
#include "stats.h"

namespace ns3 {
namespace ndn {
namespace fw {

// ns3::ndn::fw::BestRoute::TokenBucketWithPerInterfaceFairness
typedef TokenBucketWithPerInterfaceFairness<BestRoute> TokenBucketWithPerInterfaceFairnessBestRoute;
NS_OBJECT_ENSURE_REGISTERED(TokenBucketWithPerInterfaceFairnessBestRoute);

// ns3::ndn::fw::BestRoute::Stats::SatisfactionBasedPushback::TokenBucketWithPerInterfaceFairness
typedef TokenBucketWithPerInterfaceFairness< SatisfactionBasedPushback< BestRoute > > TokenBucketWithPerInterfaceFairnessSatisfactionBasedPushback;
NS_OBJECT_ENSURE_REGISTERED(TokenBucketWithPerInterfaceFairnessSatisfactionBasedPushback);


} // namespace fw
} // namespace ndn
} // namespace ns3
