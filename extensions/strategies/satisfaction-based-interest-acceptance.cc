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

#include "satisfaction-based-interest-acceptance.h"

#include <ns3/ndnSIM/model/fw/per-out-face-limits.h>
#include <ns3/ndnSIM/model/fw/best-route.h>

namespace ns3 {
namespace ndn {
namespace fw {

// ns3::ndn::fw::BestRoute::Stats::SatisfactionBasedInterestAcceptance::PerOutFaceLimits
template class PerOutFaceLimits< SatisfactionBasedInterestAcceptance<BestRoute> >;
typedef PerOutFaceLimits< SatisfactionBasedInterestAcceptance<BestRoute> > PerOutFaceLimitsSatisfactionBasedInterestAcceptanceBestRoute;
NS_OBJECT_ENSURE_REGISTERED(PerOutFaceLimitsSatisfactionBasedInterestAcceptanceBestRoute);


} // namespace fw
} // namespace ndn
} // namespace ns3
