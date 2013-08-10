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

#include "satisfaction-based-pushback.h"

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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

template<class Parent>
LogComponent SatisfactionBasedPushback<Parent>::g_log = LogComponent (SatisfactionBasedPushback<Parent>::GetLogName ().c_str ());

template<class Parent>
TypeId
SatisfactionBasedPushback<Parent>::GetTypeId (void)
{
  static TypeId tid = TypeId ((super::GetTypeId ().GetName ()+"::SatisfactionBasedPushback").c_str ())
    .SetGroupName ("Ndn")
    .template SetParent <super> ()
    .template AddConstructor <SatisfactionBasedPushback> ()

    .template AddAttribute ("GraceThreshold", "Fraction of resources that we are willing to sacrifice for \"bad traffic\"",
                            DoubleValue (0.05),
                            MakeDoubleAccessor (&SatisfactionBasedPushback::m_graceThreshold),
                            MakeDoubleChecker<double> ())

    .template AddTraceSource ("OnLimitAnnounce",  "Event called when limits are announced",
                              MakeTraceSourceAccessor (&SatisfactionBasedPushback::m_onLimitsAnnounce))

    ;
  return tid;
}

template<class Parent>
void
SatisfactionBasedPushback<Parent>::DoDispose ()
{  
  super::DoDispose ();
}

template<class Parent>
void
SatisfactionBasedPushback<Parent>::NotifyNewAggregate ()
{
  super::NotifyNewAggregate ();

  if (!m_announceEventScheduled)
    {
      if (this->m_pit != 0 && this->m_fib != 0 && this->template GetObject<Node> () != 0)
        {
          m_announceEventScheduled = true;
          UniformVariable r (0,1);
          Simulator::ScheduleWithContext (this->template GetObject<Node> ()->GetId (),
                                          Seconds (r.GetValue ()), &SatisfactionBasedPushback<Parent>::AnnounceLimits, this);
        }
    }
}

template<class Parent>
std::string
SatisfactionBasedPushback<Parent>::GetLogName ()
{
  return super::GetLogName ()+".SatisfactionBasedPushback";
}


template<class Parent>
void
SatisfactionBasedPushback<Parent>::AddFace (Ptr<Face> face)
{
  if (face->GetObject<Limits> () == 0)
    {
      NS_FATAL_ERROR ("At least per-face limits should be enabled");
      exit (1);
    }

  super::AddFace (face);
}

template<class Parent>
void
SatisfactionBasedPushback<Parent>::OnInterest (Ptr<Face> face,
                                               Ptr<Interest> interest)
{
  if (interest->GetScope () != 0)
    super::OnInterest (face, interest);
  else
    ApplyAnnouncedLimit (face, interest);
}


template<class Parent>
void
SatisfactionBasedPushback<Parent>::AnnounceLimits ()
{
  Ptr<L3Protocol> l3 = this->template GetObject<L3Protocol> ();
  NS_ASSERT (l3 != 0);

  double sumOfWeights = 0;
  double weightNormalization = 1.0;

  for (uint32_t faceId = 0; faceId < l3->GetNFaces (); faceId ++)
    {
      Ptr<Face> inFace = l3->GetFace (faceId);
      
      double unsatisfiedAbs = 0.0;
      double countAbs       = 0.0;
          
      const ndnSIM::LoadStatsNode &stats = this->GetStats (Name ());
      ndnSIM::LoadStatsNode::stats_container::const_iterator item = stats.incoming ().find (inFace);
      if (item != stats.incoming ().end ())
        {
          // item->second.count () is also available
          unsatisfiedAbs = item->second.unsatisfied ().GetStats ().get<0> ();
          countAbs       = item->second.count ().      GetStats ().get<0> ();
        }

      if (countAbs < 0.001)
        {
          continue; // don't count face if it doesn't have incoming traffic. weight will be 1 anyways
          countAbs = 1.0;
        }

      if (unsatisfiedAbs < 0)
        {
          // std::cerr << "This could be a problem with another solution\n" << std::endl;
          // may be do something else
          unsatisfiedAbs = 0;
        }

      double weight = 1.00 - std::min (1.00 - m_graceThreshold, unsatisfiedAbs / countAbs);
      
      sumOfWeights += weight;
    }
  
  if (sumOfWeights >= 1)
    {
      // disable normalization (not necessary)
      weightNormalization = 1.0;
    }
  else if (sumOfWeights > 0.001) // limit "normalization" by 1000
    {
      // sumOfWeights /= (l3->GetNFaces ());
      weightNormalization = 1 / sumOfWeights;
    }
  else
    {
      weightNormalization = 1000;
    }

  
  NS_LOG_DEBUG( "normalized weight: " << weightNormalization);
  
  for (Ptr<fib::Entry> entry = this->m_fib->Begin ();
       entry != this->m_fib->End ();
       entry = this->m_fib->Next (entry))
    {
      Ptr<Interest> announceInterest = Create<Interest> ();
      announceInterest->SetScope (0); // link-local

      double totalAllowance = 0.0;
      bool unlimited = false;
      for (fib::FaceMetricContainer::type::iterator fibFace = entry->m_faces.begin ();
           fibFace != entry->m_faces.end ();
           fibFace ++)
        {
          if (!fibFace->GetFace ()->GetObject<Limits> ()->IsEnabled ())
            {
              unlimited = true;
              break;
            }
            
          totalAllowance += fibFace->GetFace ()->GetObject<Limits> ()->GetCurrentLimit ();
          // totalAllowance += fibFace->m_face->GetObject<Limits> ()->GetMaxLimit ();
        }
      NS_LOG_DEBUG ("total: " << totalAllowance);
      
      if (unlimited)
        {
          // don't announce anything, there is no limit set
          continue;
        }
      
      for (uint32_t faceId = 0; faceId < l3->GetNFaces (); faceId ++)
        {
          Ptr<Face> inFace = l3->GetFace (faceId);

          double unsatisfiedAbs = 0.0;
          double countAbs       = 0.0;
          
          const ndnSIM::LoadStatsNode &stats = this->GetStats (Name ());
          ndnSIM::LoadStatsNode::stats_container::const_iterator item = stats.incoming ().find (inFace);
          if (item != stats.incoming ().end ())
            {
              // item->second.count () is also available
              unsatisfiedAbs = item->second.unsatisfied ().GetStats ().get<0> ();
              countAbs       = item->second.count ().      GetStats ().get<0> ();
            }

          double weight = 1.0; // if not stats available, use 1.0

          double realWeight = -1; // just for tracing purposes
          
          if (countAbs > 0.001)
            {
              if (unsatisfiedAbs < 0)
                {
                  // std::cerr << "This could be a problem with another solution\n" << std::endl;
                  // may be do something else
                  unsatisfiedAbs = 0;
                }
      
              weight = 1.00 - std::min (1.00 - m_graceThreshold, unsatisfiedAbs / countAbs);
              weight = weightNormalization * weight;

              realWeight = 1.00 - std::min (1.00, unsatisfiedAbs / countAbs);
            }
          
          NS_LOG_DEBUG( "weight: " << weight);
          // std::cout << Simulator::Now ().ToDouble(Time::S) << "s" << "\t" << Simulator::GetContext () << "\t" << weight << "\t" << realWeight
          //           << "\tunsat:" << unsatisfiedAbs << "\tcount:" << countAbs << std::endl;

          weight = static_cast<double> (std::max (0.0, weight * totalAllowance));
          
          Ptr<NameComponents> prefixWithLimit = Create<NameComponents> (entry->GetPrefix ());
          prefixWithLimit
            ->append ("limit")
            .append (boost::lexical_cast<std::string> (weight));

          NS_LOG_DEBUG ("announce: " << *prefixWithLimit);
          announceInterest->SetName (prefixWithLimit);
          // lifetime is 0

          inFace->SendInterest (announceInterest);
          m_onLimitsAnnounce (inFace, realWeight, weight/totalAllowance, weight);
        }
    }

  // this will be in the right context automatically
  Simulator::Schedule (Seconds (1.0), &SatisfactionBasedPushback::AnnounceLimits, this);
}

template<class Parent>
void
SatisfactionBasedPushback<Parent>::ApplyAnnouncedLimit (Ptr<Face> inFace,
                                                        Ptr<const Interest> interest)
{
  // Ptr<fib::Entry> fibEntry = m_fib->LongestPrefixMatch (header);
  // if (fibEntry == 0)
  //   return;

  double limit = boost::lexical_cast<double> (interest->GetName ().get (-1));
  inFace->GetObject<Limits> ()->UpdateCurrentLimit (limit);
}


} // namespace fw
} // namespace ndn
} // namespace ns3


#include <ns3/ndnSIM/model/fw/per-out-face-limits.h>
#include <ns3/ndnSIM/model/fw/best-route.h>

namespace ns3 {
namespace ndn {
namespace fw {

// ns3::ndn::fw::BestRoute::Stats::SatisfactionBasedPushback::PerOutFaceLimits
template class PerOutFaceLimits< SatisfactionBasedPushback<BestRoute> >;
typedef PerOutFaceLimits< SatisfactionBasedPushback<BestRoute> > PerOutFaceLimitsSatisfactionBasedPushbackBestRoute;
NS_OBJECT_ENSURE_REGISTERED(PerOutFaceLimitsSatisfactionBasedPushbackBestRoute);


} // namespace fw
} // namespace ndn
} // namespace ns3
