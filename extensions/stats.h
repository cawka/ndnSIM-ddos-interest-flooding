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


#ifndef NDNSIM_STATS_H
#define NDNSIM_STATS_H

#include <ns3/ndn-forwarding-strategy.h>

#include <ns3/ndnSIM/ndn.cxx/name.h>
#include <ns3/ndnSIM/utils/trie/trie.h>

#include "stats/load-stats-node.h"
#include <ns3/log.h>

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Strategy based on best route and adding statistics gathering capabilities
 */
template<class Parent>
class Stats :
    public Parent
{
  typedef Parent super;

public:
  typedef ndnSIM::trie< Name,
                        ndnSIM::non_pointer_traits< ndnSIM::LoadStatsNode >, void* > tree_type;

  static TypeId
  GetTypeId ();
    
  static std::string
  GetLogName ();

  /**
   * @brief Default constructor
   */
  Stats ();

  const ndnSIM::LoadStatsNode &
  GetStats (const Name &key) const;
  
  virtual void
  RemoveFace (Ptr<Face> face);

protected:
  // virtual void
  // DidCreatePitEntry (Ptr<Face> inFace,
  //                    Ptr<const Interest> interest,
  //                    Ptr<pit::Entry> pitEntry);

  virtual void
  DidSendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const Interest> interest,
                      Ptr<pit::Entry> pitEntry);  
  
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

  // virtual void
  // DidExhaustForwardingOptions (Ptr<Face> inFace,
  //                              Ptr<const InterestHeader> header,
  //                              Ptr<const Packet> origPacket,
  //                              Ptr<pit::Entry> pitEntry);
  
  // from Object
  void
  DoDispose ();

  void
  NotifyNewAggregate ();
  
private:
  void
  RefreshStats ();

  const ndnSIM::LoadStatsNode &
  WalkLeftRightRoot (tree_type *node);
  
private:
  static LogComponent g_log;

  tree_type m_tree;
  bool m_statsRefreshScheduled;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_STATS_H
