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

#ifndef LOAD_STATS_NODE_H
#define LOAD_STATS_NODE_H

#include "load-stats-face.h"

#include <map>
#include <ns3/ptr.h>

namespace ns3 {
namespace ndn {

class Face;

namespace ndnSIM
{

// this thing is actually put into a tree node, associated with each "name entry"

class LoadStatsNode
{
public:
  typedef std::map< ns3::Ptr<Face>, LoadStatsFace > stats_container;

  LoadStatsNode () {}
  LoadStatsNode (const LoadStatsNode &) {}

  /**
   */
  void
  Step ();

  /**
   * @brief Increment counter to incoming list
   */
  void
  AddIncoming (ns3::Ptr<Face> face);

  /**
   * @brief Increment counter to both incoming and outgoing lists, for all faces
   */
  void
  AddUnsatisfied (ns3::Ptr<Face> face);



  
  LoadStatsNode &
  operator += (const LoadStatsNode &stats);

  inline const stats_container &
  incoming () const
  {
    return m_incoming;
  }
  
  bool
  IsZero () const;
  
  bool
  operator == (const LoadStatsNode &other) const;
  
  bool
  operator != (const LoadStatsNode &other) const
  {
    return !(*this == other);
  }

  LoadStatsNode &
  operator = (const LoadStatsNode &other)
  {
    // don't do any copying at all
    return *this;
  }

  void
  RemoveFace (ns3::Ptr<Face> face);
  
private:
  stats_container m_incoming;

  friend std::ostream&
  operator << (std::ostream &os, const LoadStatsNode &node);
};
  
std::ostream&
operator << (std::ostream &os, const LoadStatsNode &node);

} // ndnSIM
} // ndn
} // ns3

#endif
