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

#ifndef SATISFACTION_BASED_INTEREST_ACCEPTANCE_H
#define SATISFACTION_BASED_INTEREST_ACCEPTANCE_H

#include "ns3/random-variable.h"
#include "ns3/double.h"
#include "ns3/ndn-fib-entry.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"

#include "stats.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Strategy implementing stats-based randomized accept for interests
 *
 * This strategy has several limitation to accept interest for further processing:
 * - if per-fib-entry limit or per-face limit is exhausted, Interest will not be accepted
 * - if the number of interests received from the incoming face is less than threshold, then no special handling
 * - if this number is greater than threshold, an Interest will be accepted with probability equal to satisfaction ratio for this incoming face (overall per-face).
 * (probability is shifted to allow small rate of acceptance (1% by default) of Interests from faces with 0 satisfaction ratio.
 */

template<class Parent>
class SatisfactionBasedInterestAcceptance :
    public Stats< Parent >
{
public:
  typedef Stats< Parent > super;
  
  static TypeId
  GetTypeId ();

  static std::string
  GetLogName ();

  /**
   * @brief Default constructor
   */
  SatisfactionBasedInterestAcceptance ()
    : m_acceptChance (0.0, 1.0)
  { }

protected:
  bool
  CanSendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const Interest> interest,
                      Ptr<pit::Entry> pitEntry);
  
private:
  static LogComponent g_log;
  
  double m_graceThreshold;
  // double m_graceAcceptProbability;
  UniformVariable m_acceptChance;
};


template<class Parent>
LogComponent SatisfactionBasedInterestAcceptance<Parent>::g_log = LogComponent (SatisfactionBasedInterestAcceptance<Parent>::GetLogName ().c_str ());

template<class Parent>
TypeId
SatisfactionBasedInterestAcceptance<Parent>::GetTypeId (void)
{
  static TypeId tid = TypeId ((super::GetTypeId ().GetName ()+"::SatisfactionBasedInterestAcceptance").c_str ())
    .SetGroupName ("Ndn")
    .template SetParent <super> ()
    .template AddConstructor <SatisfactionBasedInterestAcceptance> ()

    .AddAttribute ("GraceThreshold", "Fraction of resources that we are willing to sacrifice for \"bad traffic\"",
                            DoubleValue (0.05),
                            MakeDoubleAccessor (&SatisfactionBasedInterestAcceptance::m_graceThreshold),
                            MakeDoubleChecker<double> ())
    
    // .AddAttribute ("Threshold", "Minimum number of incoming interests to enable dropping decision",
    //                DoubleValue (0.25),
    //                MakeDoubleAccessor (&SatisfactionBasedInterestAcceptance::m_threshold),
    //                MakeDoubleChecker<double> ())
    
    // .AddAttribute ("GraceAcceptProbability", "Probability to accept Interest even though stats telling that satisfaction ratio is 0",
    //                DoubleValue (0.01),
    //                MakeDoubleAccessor (&SatisfactionBasedInterestAcceptance::m_graceAcceptProbability),
    //                MakeDoubleChecker<double> ())
    ;
  return tid;
}

template<class Parent>
std::string
SatisfactionBasedInterestAcceptance<Parent>::GetLogName ()
{
  return super::GetLogName () + ".SatisfactionBasedInterestAcceptance";
}

template<class Parent>
bool
SatisfactionBasedInterestAcceptance<Parent>::CanSendOutInterest (Ptr<Face> inFace,
                                                                 Ptr<Face> outFace,
                                                                 Ptr<const Interest> interest,
                                                                 Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << pitEntry->GetPrefix ());

  double totalAllowance = 0.0;
  bool unlimited = false;
  for (fib::FaceMetricContainer::type::iterator fibFace = pitEntry->GetFibEntry ()->m_faces.begin ();
       fibFace != pitEntry->GetFibEntry ()->m_faces.end ();
       fibFace ++)
    {
      if (!fibFace->GetFace ()->GetObject<Limits> ()->IsEnabled ())
        {
          unlimited = true;
          break;
        }
            
      totalAllowance += fibFace->GetFace ()->GetObject<Limits> ()->GetCurrentLimit ();
      // totalAllowance += fibFace->GetFace ()->GetObject<Limits> ()->GetMaxLimit ();
    }
  NS_LOG_DEBUG ("total: " << totalAllowance);

  double unsatisfiedAbs = 0.0;
  double countAbs       = 0.0;
          
  const ndnSIM::LoadStatsNode &stats = this->GetStats (Name ());
  ndnSIM::LoadStatsNode::stats_container::const_iterator item = stats.incoming ().find (inFace);
  if (item != stats.incoming ().end ())
    {
      unsatisfiedAbs = item->second.unsatisfied ().GetStats ().get<0> ();
      countAbs       = item->second.count ().      GetStats ().get<0> ();
    }
  
  if (!unlimited &&
      countAbs > m_graceThreshold * totalAllowance &&
      unsatisfiedAbs > 0)
    {
      if (unsatisfiedAbs < 0)
        {
          std::cerr << "This could be a problem with another solution\n" << std::endl;
          // may be do something else
          unsatisfiedAbs = 0;
        }

      double weight = 1.00 - std::min (1.00, unsatisfiedAbs / countAbs);

      double chance = m_acceptChance.GetValue ();
      if (chance > weight) // too bad for the Interest
        {
          return false;
        }
    }

  // should be PerOutFaceLimits
  return super::CanSendOutInterest (inFace, outFace, interest, pitEntry);
}


} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // SATISFACTION_BASED_INTEREST_ACCEPTANCE_H
