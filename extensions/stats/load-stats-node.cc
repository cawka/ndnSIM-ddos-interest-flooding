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

#include "load-stats-node.h"
#include "ns3/ndn-face.h"
#include "ns3/log.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.LoadStatsNode");

namespace ns3 {
namespace ndn {
namespace ndnSIM {

void
LoadStatsNode::Step ()
{
  NS_LOG_FUNCTION (this);
  
  for (stats_container::iterator item = m_incoming.begin ();
       item != m_incoming.end ();
       item ++)
    {
      item->second.Step ();
    }
}

void
LoadStatsNode::AddIncoming (ns3::Ptr<Face> face)
{
  m_incoming [face].count ()++;
}

void
LoadStatsNode::AddUnsatisfied (ns3::Ptr<Face> face)
{
  m_incoming [face].unsatisfied ()++;
}


LoadStatsNode &
LoadStatsNode::operator += (const LoadStatsNode &stats)
{
  NS_LOG_FUNCTION (this << &stats);
  
  // aggregate incoming
  for (stats_container::const_iterator item = stats.m_incoming.begin ();
       item != stats.m_incoming.end ();
       item ++)
    {
      m_incoming [item->first] += item->second;
    }

  return *this;
}

bool
LoadStatsNode::IsZero () const
{
  bool zero = true;
  for (stats_container::const_iterator item = m_incoming.begin ();
       item != m_incoming.end ();
       item ++)
    {
      zero &= item->second.IsZero ();
    }
  
  return zero;  
}


void
LoadStatsNode::RemoveFace (ns3::Ptr<Face> face)
{
  NS_LOG_FUNCTION (this);
  m_incoming.erase (face);
}

bool
LoadStatsNode::operator == (const LoadStatsNode &other) const
{
  if (other.m_incoming.size () > 0)
    return false;

  return IsZero ();
}

std::ostream&
operator << (std::ostream &os, const LoadStatsNode &node)
{
  for (LoadStatsNode::stats_container::const_iterator item = node.m_incoming.begin ();
       item != node.m_incoming.end ();
       item ++)
    {
      if (item != node.m_incoming.begin ()) os << ", ";
      os << *(item->first) << ":" << item->second;
    }
  os << "\n";
  
  return os;
}


} // namespace ndnSIM
} // namespace ndn
} // namespace ns3
