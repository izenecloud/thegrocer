#ifndef THEGROCER_KMEANS_H_ 
#define THEGROCER_KMEANS_H_
#include "basis.hpp"
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>

namespace thegrocer {

  //
  // This class stores all the points available in the model
  //
  class PointsSpace{

    //
    // Dump collection of points
    //
    friend std::ostream& operator << (std::ostream& os, PointsSpace & ps){

      PointId i = 0;
      BOOST_FOREACH(Points::value_type p, ps.points__){     
	    os << "point["<<i++<<"]=" << p << std::endl;
      }
      return os;
    };

  public:

    PointsSpace(PointId num_points, Dimensions num_dimensions) 
      : num_points__(num_points), num_dimensions__(num_dimensions)
    {init_points();};

    inline PointId getNumPoints() const {return num_points__;}
    inline PointId getNumDimensions() const {return num_dimensions__;}
    inline const Point& getPoint(PointId pid) const { return points__[pid];}
    inline Point& getPoint(PointId pid) { return points__[pid];}
 
  private:
    //
    // Init collection of points
    //
    void init_points()
    {
        for (PointId i=0; i < num_points__; i++){
            Point p;
            for (Dimensions d=0 ; d < num_dimensions__; d++)
            { 
                p.push_back( rand() % 100 ); 
            }     
            points__.push_back(p);

            std::cout << "pid[" << i << "]= (" << p << ")" <<std::endl;; 
        }
    }
    
    PointId num_points__;
    Dimensions num_dimensions__;
    Points points__;
  };

  // 
  //  This class represents a cluster
  // 
  class Clusters {

  private:
   
    ClusterId num_clusters__;    // number of clusters
    PointsSpace& ps__;           // the point space
    Dimensions num_dimensions__; // the dimensions of vectors
    PointId num_points__;        // total number of points
    ClustersToPoints clusters_to_points__;
    PointsToClusters points_to_clusters__;
    Centroids centroids__;

    //
    // Zero centroids
    //
    void zero_centroids()
    {
        BOOST_FOREACH(Centroids::value_type& centroid, centroids__){
          BOOST_FOREACH(Point::value_type& d, centroid){
            d = 0.0;
          }
        }
    }

    void compute_centroids()
    {
        Dimensions i;
        ClusterId cid = 0;
        PointId num_points_in_cluster;
        // For each centroid
        BOOST_FOREACH(Centroids::value_type& centroid, centroids__){

          num_points_in_cluster = 0;

          // For earch PointId in this set
          BOOST_FOREACH(SetPoints::value_type pid, 
                clusters_to_points__[cid]){
        
            Point p = ps__.getPoint(pid);
            //std::cout << "(" << p << ")";
            for (i=0; i<num_dimensions__; i++)
              centroid[i] += p[i];	
            num_points_in_cluster++;
          }
          //
          // if no point in the clusters, this goes to inf (correct!)
          //
          for (i=0; i<num_dimensions__; i++)
            centroid[i] /= num_points_in_cluster;	
          cid++;
        }
    }
    
    //
    // Initial partition points among available clusters
    //
    void initial_partition_points()
    {
        ClusterId cid;
        
        for (PointId pid = 0; pid < ps__.getNumPoints(); pid++){
          
          cid = pid % num_clusters__;

          points_to_clusters__[pid] = cid;
          clusters_to_points__[cid].insert(pid);
        }
    }

  public:
   
    //
    // Dump ClustersToPoints
    //
    friend std::ostream& operator << (std::ostream& os, Clusters & cl){
      
      ClusterId cid = 0;
      BOOST_FOREACH(ClustersToPoints::value_type set, cl.clusters_to_points__){
	    os << "Cluster["<<cid<<"]=(";
	    BOOST_FOREACH(SetPoints::value_type pid, set){
	        Point p = cl.ps__.getPoint(pid);
	        os << "(" << p << ")";
	    }
	    os << ")" << std::endl;
	    cid++;
      }
      return os;
    }

    const ClustersToPoints& getClustersToPoints() const {
        return clusters_to_points__;
    }
    

    Clusters(ClusterId num_clusters, PointsSpace & ps) 
      : num_clusters__(num_clusters), ps__(ps), 
	num_dimensions__(ps.getNumDimensions()),
	num_points__(ps.getNumPoints()),
	points_to_clusters__(num_points__, 0){

      ClusterId i = 0;
      Dimensions dim;
      for (; i < num_clusters; i++){
	    Point point;   // each centroid is a point
	    for (dim=0; dim<num_dimensions__; dim++) 
	        point.push_back(0.0);
	    SetPoints set_of_points;

        // init centroids
        centroids__.push_back(point);  

        // init clusterId -> set of points
        clusters_to_points__.push_back(set_of_points);
        // init point <- cluster
      }
      /*
      std::cout << "Centroids" 
		<<std::endl<< centroids__;
      std::cout << "PointsToClusters" 
		<<std::endl<< points_to_clusters__;
      std::cout << "ClustersToPoints" 
		<<std::endl<< clusters_to_points__;
      */
    }

    Distance distance(const Point& x, const Point& y)
    {
        Distance total = 0.0;
        Distance diff;
        
        Point::const_iterator cpx=x.begin(); 
        Point::const_iterator cpy=y.begin();
        Point::const_iterator cpx_end=x.end();
        for(;cpx!=cpx_end;++cpx,++cpy){
          diff = *cpx - *cpy;
          total += (diff * diff); 
        }
        return total;  // no need to take sqrt, which is monotonic
    }
    
    //
    // k-means
    //
    void k_means ()
    {
        bool move;
        bool some_point_is_moving = true;
        unsigned int num_iterations = 0;
        PointId pid;
        ClusterId cid, to_cluster;
        Distance d, min;
        

        //
        // Initial partition of points
        //
        initial_partition_points();

        //
        // Until not converge
        //
        while (some_point_is_moving){

          std::cout << std::endl << "*** Num Iterations " 
            << num_iterations  << std::endl << std::endl ;;

          some_point_is_moving = false;

          compute_centroids();
          //      std::cout << "Centroids" << std::endl << centroids__;      

          //
          // for each point
          //
          for (pid=0; pid<num_points__; pid++){
        
            // distance from current cluster
            min = distance(centroids__[points_to_clusters__[pid]], ps__.getPoint(pid));

            std::cout << "pid[" << pid << "] in cluster=" 
                  << points_to_clusters__[pid] 
                  << " with distance=" << min << std::endl;

            //
            // foreach centroid
            //
            cid = 0; 
            move = false;
            BOOST_FOREACH(Centroids::value_type c, centroids__){
              

              d = distance(c, ps__.getPoint(pid));
              if (d < min){
                min = d;
                move = true;
                to_cluster = cid;

                // remove from current cluster
                clusters_to_points__[points_to_clusters__[pid]].erase(pid);

                some_point_is_moving = true;
                std::cout << "\tcluster=" << cid 
                      << " closer, dist=" << d << std::endl;	    
              }
              cid++;
            }
            
            //
            // move towards a closer centroid 
            //
            if (move){
              
              // insert
              points_to_clusters__[pid] = to_cluster;
              clusters_to_points__[to_cluster].insert(pid);
              std::cout << "\t\tmove to cluster=" << to_cluster << std::endl;
            }
          }      

          num_iterations++;
        } // end while (some_point_is_moving)

        //std::cout << std::endl << "Final clusters" << std::endl;
        //std::cout << clusters_to_points__;
    }
  };
}

#endif

