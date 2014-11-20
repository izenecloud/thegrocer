#ifndef THEGROCER_BASIS_H_ 
#define THEGROCER_BASIS_H_
#include <set>
#include <vector>
#include <iostream>
#include <boost/foreach.hpp>

namespace thegrocer {
  typedef float Coord;            // a coordinate
  typedef double Distance;         // distance
  typedef uint32_t Dimensions; // how many dimensions
  typedef uint32_t PointId;    // the id of this point
  typedef uint32_t ClusterId;  // the id of this cluster

  typedef std::vector<Coord> Point;    // a point (a centroid)
  typedef std::vector<Point> Points;   // collection of points

  typedef std::set<PointId> SetPoints; // set of points
  
  // ClusterId -> (PointId, PointId, PointId, .... )
  typedef std::vector<SetPoints> ClustersToPoints;
  // PointId -> ClusterId
  typedef std::vector<ClusterId> PointsToClusters; 
  // coll of centroids
  typedef std::vector<Point> Centroids;

  std::ostream& operator << (std::ostream& os, Point& p)
  {
    BOOST_FOREACH(Point::value_type d, p){ os << d << " "; }    
    return os;
  }

  //
  // Dump collection of Points and Centroids
  //
  std::ostream& operator << (std::ostream& os, Points& cps)
  {
    BOOST_FOREACH(Points::value_type p, cps){ os<<p <<std::endl;}
    return os;
  }

  //
  // Dump a Set of points
  //
  std::ostream& operator << (std::ostream& os, SetPoints & sp)
  {
    BOOST_FOREACH(SetPoints::value_type pid, sp){     
      os << "pid=" << pid << " " ;
    }
    return os;
  }
    
  

  //
  // Dump ClustersToPoints
  //
  std::ostream& operator << (std::ostream& os, ClustersToPoints & cp)
  {
    ClusterId cid = 0;
    BOOST_FOREACH(ClustersToPoints::value_type set, cp){
      os << "clusterid["  << cid << "]" << "=(" 
	 << set << ")" << std::endl; 
      cid++;
    }
    return os;
  }

  //
  // Dump ClustersToPoints
  //
  std::ostream& operator << (std::ostream& os, PointsToClusters & pc)
  {
    PointId pid = 0;
    BOOST_FOREACH(PointsToClusters::value_type cid, pc){
      
      std::cout << "pid[" << pid << "]=" << cid << std::endl;      
      pid ++;
    }
    return os;
  }

};


#endif

