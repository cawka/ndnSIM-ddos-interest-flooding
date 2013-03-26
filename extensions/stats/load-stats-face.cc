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

#include "load-stats-face.h"

#include "ns3/log.h"

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>

using namespace boost::mpl;
using namespace boost::tuples;

NS_LOG_COMPONENT_DEFINE ("ndn.LoadStatsFace");

namespace ns3 {
namespace ndn {
namespace ndnSIM {

std::ostream &
operator << (std::ostream &os, const LoadStats::stats_tuple &tuple);

void
LoadStatsFace::Step ()
{
  m_unsatisfied.Step (0.125 /* 1/8 */); // 0.875
  m_count.Step (0.015625 /* 1/32 */);   // 0.96875
  // m_count.Step (0.03125 /* 1/32 */);   // 0.96875

  // m_unsatisfied.Step (0.0625 /* 1/16 */); 
  // m_count.Step (0.00390625 /* 1/16^2 */);
}

LoadStatsFace &
LoadStatsFace::operator += (const LoadStatsFace &load)
{
  m_count       += load.m_count;
  m_unsatisfied += load.m_unsatisfied;

  return *this;
}

bool
LoadStatsFace::IsZero () const
{
  return m_count.IsZero () && m_unsatisfied.IsZero ();
}

struct print_tuple
{
  print_tuple (const LoadStats::stats_tuple &tuple, std::ostream &os)
    : tuple_ (tuple)
    , os_ (os)
  {
  }
  
  const LoadStats::stats_tuple &tuple_;
  std::ostream &os_;
    
  template< typename U >
  void operator () (U)
  {
    os_ << tuple_.get< U::value > () << " ";
  }
};


std::ostream &
operator << (std::ostream &os, const LoadStats::stats_tuple &tuple)
{
  for_each< range_c<int, 0, length<LoadStats::stats_tuple>::value> >
    (print_tuple (tuple, os));
  
  return os;
}

std::ostream &
operator << (std::ostream &os, const LoadStatsFace &stats)
{
  os << "count: " << stats.count () << "/ unsatisfied: " << stats.unsatisfied ();
  return os;
}

} // namespace ndnSIM
} // namespace ndn
} // namespace ns3

