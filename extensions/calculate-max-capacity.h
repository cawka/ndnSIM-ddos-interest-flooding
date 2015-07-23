/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
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

#ifndef CALCULATE_MAX_CAPACITY_H
#define CALCULATE_MAX_CAPACITY_H

#define BOOST_NO_CXX11_RVALUE_REFERENCES
// #include <boost/graph/push_relabel_max_flow.hpp>
// #include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>

#include <ns3/ndnSIM/model/ndn-global-router.h>

template<class PropertyMap>
class dfs_time_visitor : public boost::default_dfs_visitor {
public:
  dfs_time_visitor (PropertyMap &map, bool &valid)
  : m_map (map)
  , m_valid (valid)
  {
  }
  
  template < typename Vertex, typename Graph >
  void finish_vertex (Vertex &v, Graph & g)
  {
    using namespace boost;

    typename graph_traits<Graph>::out_edge_iterator edge, endEdge;
    tie (edge, endEdge) = out_edges (v, g);
    if (edge == endEdge)
      {
        double rank = get (vertex_rank, g, v);
        
        // cout << "*" << v << " (" << rank << ")" << endl;
        return;
      }

    // Ptr<Node> node = get (vertex_name, g, v);
    // if (Names::FindName (node).find ("leaf") != string::npos)
    //   {
    //     NS_FATAL_ERROR ("violating valley policy");
    //   }
    
    double counter = 0;
    for (; edge != endEdge; edge++)
      {
        double capacity = get (edge_capacity, g, *edge);

        Vertex v2 = target (*edge, g);
        double rank = get (vertex_rank, g, v2);

        if (rank > capacity)
          {
            m_valid = false;
            counter = -std::numeric_limits<double>::max ();
            
            // cout << v << " <=> " << v2 << " (cannot push " << rank << " using capacity " << capacity << ")" << endl;
          }
        else
          {
            counter += rank;
            // cout << v << " <=> " << v2 << " (" << capacity << ")" << endl;
          }
      }

    put (m_map, v, counter);
    // cout << ">> " << v << " (rank: " << counter << "," << m_valid << ")" << endl;
  }

  PropertyMap &m_map;
  bool &m_valid;
};


inline
bool
checkNoCongestion (const NodeContainer &sources, const NodeContainer &targets, double share)
{
  using namespace boost;

  typedef adjacency_list_traits<vecS, vecS, directedS> Traits;

  typedef property< vertex_name_t, Ptr<Node>,
                    property <vertex_rank_t, double > > nodeProperty;
  typedef property< edge_capacity_t, double >  edgeProperty;
  
  typedef adjacency_list< vecS, vecS, directedS, 
                          nodeProperty, edgeProperty > Graph;

  Graph g;

  std::map< Ptr<Node>, Traits::vertex_descriptor > verts;

  auto addEdge = [&](Traits::vertex_descriptor v1, Traits::vertex_descriptor v2, double edgeCapacity) {
    Traits::edge_descriptor edge;
    bool ok;
    tie (edge, ok) = add_edge (v2, v1, edgeProperty (edgeCapacity), g);
    NS_ASSERT (ok);

    // Ptr<Node> n1 = get (vertex_name, g, v1);
    // Ptr<Node> n2 = get (vertex_name, g, v2);

    // cout << v1 << ", " << v2 << endl;
    // cout << (n1!=0? Names::FindName(n1):"?") << " <=> " << (n2!=0? Names::FindName(n2):"?") << endl;
  };
  
  std::list< Ptr<Node> > nodeList;
  for (ns3::NodeList::Iterator node = sources.Begin (); node != sources.End (); node++)
    {
      nodeList.push_back (*node);
      verts[ *node ] = add_vertex (nodeProperty (*node), g);
      put (vertex_rank, g, verts[ *node ], share);
    }

  while (nodeList.size () > 0)
    {
      Ptr<Node> node = *nodeList.begin ();
      nodeList.pop_front ();
      
      Ptr<ndn::Fib> fib = node->GetObject <ndn::Fib> ();
      
      Ptr<const fib::Entry> entry = fib->Begin ();
      if (entry == 0) continue;
      
      Ptr<NetDeviceFace> face = DynamicCast<NetDeviceFace> (entry->m_faces.begin ()->GetFace ());
      if (face == 0) continue; 
      
      DataRateValue dataRate; DynamicCast<NetDeviceFace> (face)->GetNetDevice ()->GetAttribute ("DataRate", dataRate);

      Ptr<Node> other = face->GetNetDevice ()->GetChannel ()->GetDevice (0)->GetNode ();
      if (other == node)
        other = face->GetNetDevice ()->GetChannel ()->GetDevice (1)->GetNode ();

      if (verts.find (other) == verts.end ())
        {
          nodeList.push_back (other);
          verts[ other ] = add_vertex (nodeProperty (other), g);
        }
      
      addEdge (verts[ node ], verts[ other ], dataRate.Get ().GetBitRate () / 8.0 / 1024); // in kilobytes
    }

  property_map<Graph, vertex_rank_t>::type ranks = get(vertex_rank, g);

  bool isValid = true;  
  dfs_time_visitor< property_map<Graph, vertex_rank_t>::type > vis (ranks, isValid);
  depth_first_search (g, root_vertex(verts[ targets.Get (0) ]).visitor(vis));

  // cout << "Share: " << share << " valid: " << isValid;
  // if (isValid)
  //   cout << " (total: " << get (vertex_rank, g, verts[ targets.Get (0) ]) << ")";
  // cout << endl;

  return isValid;
}

inline
double
calculateNonCongestionFlows (const NodeContainer &sources, const NodeContainer &targets)
{
  double share = 1;

  while (checkNoCongestion (sources, targets, share))
    {
      share *= 2;
    }

  // cout << share << endl;
  // max_boundary = share
  // min_boundary = share/2

  double shareMin = share/2;
  double shareMax = share;

  while (shareMax - shareMin > 0.1)
    {
      bool ok = checkNoCongestion (sources, targets, (shareMin+shareMax)/2);
      if (ok)
        shareMin = (shareMin+shareMax)/2;
      else
        shareMax = (shareMin+shareMax)/2;
    }

  return shareMin;
}


template <class Nodes>
class ns3node_writer {
public:
  ns3node_writer(Nodes _nodes) : nodes(_nodes) {}
    
  template <class VertexOrEdge>
  void operator()(std::ostream& out, const VertexOrEdge& v) const
  {
    string name = Names::FindName (nodes[v]);
    string color = "gray";
    string size = "0.1";
    if (name.find ("bb-") == 0)
      {
        size = "0.4";
        color = "black";
      }
    else if (name.find ("gw-") == 0)
      {
        size = "0.3";
        color = "gray";
      }
    else if (name.find ("leaf-") == 0)
      {
        color = "brown";
      }
    else if (name.find ("producer-") == 0)
      {
        size = "0.3";
        color = "blue";
      }
    else if (name.find ("good-") == 0)
      {
        color = "green";
      }
    else if (name.find ("evil-") == 0)
      {
        color = "red";
      }
    else
      {
        // use default gray color
      }
    // if (name == "good-leaf-12923")
    //   out << "[shape=\"circle\",width=" << size << ",label=\"" << name << "\",style=filled,fillcolor=\"" << color << "\"]";
    // else
    out << "[shape=\"circle\",width=" << size << ",label=\"\",style=filled,fillcolor=\"" << color << "\"]";
  }
private:
  Nodes nodes;
};

template <class Nodes>
inline ns3node_writer<Nodes>
make_ns3node_writer(Nodes n) {
  return ns3node_writer<Nodes> (n);
}


inline
void
saveActualGraph (const std::string &file, const NodeContainer &sources)
{
  using namespace boost;

  typedef adjacency_list_traits<vecS, vecS, directedS> Traits;

  typedef property< vertex_name_t, Ptr<Node> > nodeProperty;
  typedef property< edge_capacity_t, double >  edgeProperty;
  
  typedef adjacency_list< vecS, vecS, directedS, 
                          nodeProperty, edgeProperty > Graph;

  Graph g;

  std::map< Ptr<Node>, Traits::vertex_descriptor > verts;

  auto addEdge = [&](Traits::vertex_descriptor v1, Traits::vertex_descriptor v2, double edgeCapacity) {
    Traits::edge_descriptor edge;
    bool ok;
    tie (edge, ok) = add_edge (v2, v1, edgeProperty (edgeCapacity), g);
    NS_ASSERT (ok);
  };
  
  std::list< Ptr<Node> > nodeList;
  for (ns3::NodeList::Iterator node = sources.Begin (); node != sources.End (); node++)
    {
      nodeList.push_back (*node);
      verts[ *node ] = add_vertex (nodeProperty (*node), g);
    }

  while (nodeList.size () > 0)
    {
      Ptr<Node> node = *nodeList.begin ();
      nodeList.pop_front ();
      
      Ptr<ndn::Fib> fib = node->GetObject <ndn::Fib> ();
      
      Ptr<const fib::Entry> entry = fib->Begin ();
      if (entry == 0) continue;
      
      Ptr<NetDeviceFace> face = DynamicCast<NetDeviceFace> (entry->m_faces.begin ()->GetFace ());
      if (face == 0) continue; 
      
      DataRateValue dataRate; DynamicCast<NetDeviceFace> (face)->GetNetDevice ()->GetAttribute ("DataRate", dataRate);

      Ptr<Node> other = face->GetNetDevice ()->GetChannel ()->GetDevice (0)->GetNode ();
      if (other == node)
        other = face->GetNetDevice ()->GetChannel ()->GetDevice (1)->GetNode ();

      if (verts.find (other) == verts.end ())
        {
          nodeList.push_back (other);
          verts[ other ] = add_vertex (nodeProperty (other), g);
        }
      
      addEdge (verts[ other ], verts[ node ], dataRate.Get ().GetBitRate () / 8.0 / 1024); // in kilobytes
    }

  cout << "Saving graph to: " << file << endl;
  
  ofstream of (file.c_str ());
  property_map<Graph, vertex_name_t>::type nodes = get (vertex_name, g);
  write_graphviz(of, g, make_ns3node_writer (nodes));
}

#endif // CALCULATE_MAX_CAPACITY_H
