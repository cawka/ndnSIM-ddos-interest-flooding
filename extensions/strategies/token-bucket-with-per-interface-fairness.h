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


#ifndef TOKEN_BUCKET_WITH_PER_INTERFACE_FAIRNESS_H
#define TOKEN_BUCKET_WITH_PER_INTERFACE_FAIRNESS_H

#include <ns3/event-id.h>
#include "utils/ndn-pit-queue.h"

#include <ns3/ndn-forwarding-strategy.h>

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Strategy implementing per-FIB entry limits
 */
template<class Parent>
class TokenBucketWithPerInterfaceFairness :
    public Parent
{
private:
  typedef Parent super;

public:
  static TypeId
  GetTypeId ();

  static std::string
  GetLogName ();

  TokenBucketWithPerInterfaceFairness () { }

  // overriding forwarding strategy events
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

  virtual void
  AddFace (Ptr<Face> face);
  
  virtual void
  RemoveFace (Ptr<Face> face);
  
protected:
  virtual bool
  TrySendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const Interest> interest,
                      Ptr<pit::Entry> pitEntry);
  
  virtual void
  WillSatisfyPendingInterest (Ptr<Face> inFace,
                              Ptr<pit::Entry> pitEntry);
  
private:
  void
  ProcessFromQueue ();
    
private:
  std::string m_limitType;

  typedef std::map< Ptr<Face>, PitQueue > PitQueueMap;
  PitQueueMap m_pitQueues; // per-outgoing face pit queue
};


} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // TOKEN_BUCKET_WITH_PER_INTERFACE_FAIRNESS_H
