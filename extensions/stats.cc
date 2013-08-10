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

#include "stats.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"


#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

// NS_LOG_COMPONENT_DEFINE ("ndn.fw.Stats");

namespace ns3 {
namespace ndn {

using namespace ndnSIM;

namespace fw {

template<class Parent>
LogComponent Stats<Parent>::g_log = LogComponent (Stats<Parent>::GetLogName ().c_str ());


template<class Parent>
TypeId
Stats<Parent>::GetTypeId ()
{
  static TypeId tid = TypeId ((Parent::GetTypeId ().GetName ()+"::Stats").c_str ())
    .SetGroupName ("Ndn")
    .template SetParent<Parent> ()
    .template AddConstructor< Stats<Parent> > ()
    ;
  
  return tid;
}

template<class Parent>
Stats<Parent>::Stats ()
  : m_tree (name::Component ())
  , m_statsRefreshScheduled (false)
{
}

template<class Parent>
std::string
Stats<Parent>::GetLogName ()
{
  return super::GetLogName () + ".Stats";
}

template<class Parent>
void
Stats<Parent>::DoDispose ()
{
  m_statsRefreshScheduled = false;
  super::DoDispose ();
}

template<class Parent>
void
Stats<Parent>::NotifyNewAggregate ()
{
  if (!m_statsRefreshScheduled)
    {
      Ptr<Node> node = this->template GetObject<Node> ();
      if (node != 0)
        {
          m_statsRefreshScheduled = true;
          Simulator::ScheduleWithContext (node->GetId (), Seconds (1.0), &Stats<Parent>::RefreshStats, this);
        }
    }

  super::NotifyNewAggregate ();
}

// template<class Parent>
// void
// Stats<Parent>::DidCreatePitEntry (Ptr<Face> inFace,
//                                   Ptr<const Interest> interest,
//                                   Ptr<pit::Entry> pitEntry)
// {
//   // NS_LOG_FUNCTION (inFace);
//   super::DidCreatePitEntry (inFace, interest, pitEntry);

//   std::pair<tree_type::iterator, bool> item = m_tree.insert (interest->GetName ().cut (1), LoadStatsNode ());
//   item.first->payload ().AddIncoming (inFace);
// }

template<class Parent>
void
Stats<Parent>::DidSendOutInterest (Ptr<Face> inFace,
                                   Ptr<Face> outFace,
                                   Ptr<const Interest> interest,
                                   Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (inFace);
  super::DidSendOutInterest (inFace, outFace, interest, pitEntry);

  std::pair<tree_type::iterator, bool> item = m_tree.insert (interest->GetName ().getPrefix (interest->GetName ().size ()-1),
                                                             LoadStatsNode ());
  item.first->payload ().AddIncoming (inFace);
}

template<class Parent>
void
Stats<Parent>::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (pitEntry->GetPrefix ());

  std::pair<tree_type::iterator, bool> item = m_tree.insert (pitEntry->GetPrefix ().getPrefix (pitEntry->GetPrefix ().size ()-1),
                                                             LoadStatsNode ());
  BOOST_FOREACH (const pit::IncomingFace &inFace, pitEntry->GetIncoming ())
    {
      item.first->payload ().AddUnsatisfied (inFace.m_face);
    }

  super::WillEraseTimedOutPendingInterest (pitEntry);
}

// template<class Parent>
// void
// Stats<Parent>::DidExhaustForwardingOptions (Ptr<Face> inFace,
//                                             Ptr<const Interest> interest,
//                                             Ptr<pit::Entry> pitEntry)
// {
//   // NS_LOG_FUNCTION (inFace);

//   // inFace is included in pitEntry->GetIncoming ()
//   std::pair<tree_type::iterator, bool> item = m_tree.insert (pitEntry->GetPrefix ().cut (1), LoadStatsNode ());
//   BOOST_FOREACH (const pit::IncomingFace &inFace, pitEntry->GetIncoming ())
//     {
//       item.first->payload ().AddUnsatisfied (inFace.m_face);
//     }

//   super::DidExhaustForwardingOptions (inFace, interest, pitEntry);
// }


template<class Parent>
void
Stats<Parent>::RefreshStats ()
{
  // NS_LOG_FUNCTION (this);
  if (!m_statsRefreshScheduled) return;

  // walking the tree, aggregating and stepping on every node, starting the leaves

  WalkLeftRightRoot (&m_tree);
  m_tree.payload ().Step ();
  // NS_LOG_DEBUG ("[" << m_tree.key () << "] " << m_tree.payload ());  

  Simulator::Schedule (Seconds (1.0), &Stats<Parent>::RefreshStats, this);
}

template<class Parent>
void
Stats<Parent>::RemoveFace (Ptr<Face> face)
{
  tree_type::recursive_iterator item (&m_tree), end;
  for (; item != end; item ++)
    {
      item->payload ().RemoveFace (face);
    }

  super::RemoveFace (face);
}


template<class Parent>
const ndnSIM::LoadStatsNode &
Stats<Parent>::GetStats (const Name &key) const
{
  tree_type::iterator foundItem, lastItem;
  bool reachLast;
  boost::tie (foundItem, reachLast, lastItem) = const_cast<tree_type&> (m_tree).find (key);

  return lastItem->payload ();
}

template<class Parent>
const ndnSIM::LoadStatsNode&
Stats<Parent>::WalkLeftRightRoot (tree_type *node)
{
  tree_type::point_iterator item (*node), end;

  while (item != end)
    {
      node->payload () += WalkLeftRightRoot (&*item);
      item->payload ().Step ();

      // NS_LOG_DEBUG ("[" << item->key () << "] " << item->payload ());

      tree_type::point_iterator prune_iterator = item;
      item++;

      prune_iterator->prune_node ();
    }
  
  return node->payload ();
}


} // namespace fw
} // namespace ndn
} // namespace ns3


#include <ns3/ndnSIM/model/fw/best-route.h>

namespace ns3 {
namespace ndn {
namespace fw {

// ns3::ndn::fw::BestRoute::Stats::DynamicLimits::SimpleLimits
template class Stats< BestRoute >;
typedef Stats< BestRoute > StatsBestRoute;
NS_OBJECT_ENSURE_REGISTERED(StatsBestRoute);


} // namespace fw
} // namespace ndn
} // namespace ns3
