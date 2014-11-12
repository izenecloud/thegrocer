#ifndef THEGROCER_CLUSTERING_H_ 
#define THEGROCER_CLUSTERING_H_
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
      BOOST_FOREACH(Points::value_type p, ps.points_){     
	    os << "point["<<i++<<"]=" << p << std::endl;
      }
      return os;
    };

  public:

    PointsSpace(PointId num_points, Dimensions num_dimensions) 
      : num_points_(num_points), num_dimensions_(num_dimensions)
    {init_points();};

    inline PointId GetNumPoints() const {return num_points_;}
    inline PointId GetNumDimensions() const {return num_dimensions_;}
    inline const Point& GetPoint(PointId pid) const { return points_[pid];}
    inline Point& GetPoint(PointId pid) { return points_[pid];}
 
  private:
    //
    // Init collection of points
    //
    void init_points()
    {
        for (PointId i=0; i < num_points_; i++){
            Point p;
            for (Dimensions d=0 ; d < num_dimensions_; d++)
            { 
                p.push_back( rand() % 100 ); 
            }     
            points_.push_back(p);

            //std::cout << "pid[" << i << "]= (" << p << ")" <<std::endl;; 
        }
    }
    
    PointId num_points_;
    Dimensions num_dimensions_;
    Points points_;
  };

  // 
  //  This class represents a cluster
  // 
  class Clusters {

  private:
   
    ClusterId num_clusters_;    // number of clusters
    PointsSpace& ps_;           // the point space
    Dimensions num_dimensions_; // the dimensions of vectors
    PointId num_points_;        // total number of points
    ClustersToPoints clusters_to_points_;
    PointsToClusters points_to_clusters_;
    Centroids centroids_;


    void compute_centroids()
    {
        Dimensions i;
        ClusterId cid = 0;
        PointId num_points_in_cluster;
        BOOST_FOREACH(Centroids::value_type& centroid, centroids_){
          num_points_in_cluster = 0;
          BOOST_FOREACH(SetPoints::value_type pid, clusters_to_points_[cid]){
            Point p = ps_.GetPoint(pid);
            for (i=0; i<num_dimensions_; i++)
              centroid[i] += p[i];	
            num_points_in_cluster++;
          }
          for (i=0; i<num_dimensions_; i++)
            centroid[i] /= num_points_in_cluster;	
          cid++;
        }
    }
    
    void initial_partition_points()
    {
        points_to_clusters_.resize(ps_.GetNumPoints());
        clusters_to_points_.resize(num_clusters_);
        centroids_.push_back(ps_.GetPoint(0));
        SetPoints sp;
        for(PointId i=1;i<ps_.GetNumPoints();i++)
        {
            sp.insert(i);
        }
        while(true)
        {
            if(centroids_.size()==num_clusters_) break;
            std::pair<double, PointId> max_dist(0.0, 0);
            for(auto pid : sp) {
                double min_dist = -1.0;
                for(auto& cent : centroids_) {
                    double dist = distance(ps_.GetPoint(pid), cent);
                    if(min_dist<0.0 || dist<min_dist) min_dist = dist;
                }
                if(min_dist>max_dist.first)
                {
                    max_dist.first = min_dist;
                    max_dist.second = pid;
                }
            }
            centroids_.push_back(ps_.GetPoint(max_dist.second));
            sp.erase(max_dist.second);
        }
        //for(ClusterId i=0;i<num_clusters_;i++)
        //{
        //    centroids_.push_back(ps_.GetPoint(i));
        //}
        for (PointId pid = 0; pid < ps_.GetNumPoints(); pid++){
            std::pair<double, ClusterId> select(-1.0, 0);
            for(ClusterId i=0;i<num_clusters_;i++)
            {
                double dist = distance(ps_.GetPoint(pid), centroids_[i]);
                if(select.first<0.0 || dist<select.first)
                {
                    select.first = dist;
                    select.second = i;
                }
            }
            ClusterId cid = select.second;
            points_to_clusters_[pid] = cid;
            clusters_to_points_[cid].insert(pid);
        }
        //ClusterId i = 0;
        //Dimensions dim;
        //for (; i < num_clusters_; i++){
        //    Point point;   // each centroid is a point
        //    for (dim=0; dim<num_dimensions_; dim++) 
        //        point.push_back(0.0);
        //    SetPoints set_of_points;
        //    centroids_.push_back(point);  
        //    clusters_to_points_.push_back(set_of_points);
        //}
        //ClusterId cid;
        //for (PointId pid = 0; pid < ps_.GetNumPoints(); pid++){
        //  cid = pid % num_clusters_;
        //  points_to_clusters_[pid] = cid;
        //  clusters_to_points_[cid].insert(pid);
        //}
    }

  public:
   
    //
    // Dump ClustersToPoints
    //
    friend std::ostream& operator << (std::ostream& os, Clusters & cl){
      
      ClusterId cid = 0;
      BOOST_FOREACH(ClustersToPoints::value_type set, cl.clusters_to_points_){
	    os << "Cluster["<<cid<<"]=(";
	    BOOST_FOREACH(SetPoints::value_type pid, set){
	        Point p = cl.ps_.GetPoint(pid);
	        os << "(" << p << ")";
	    }
	    os << ")" << std::endl;
	    cid++;
      }
      return os;
    }

    const ClustersToPoints& GetClustersToPoints() const {
        return clusters_to_points_;
    }
    

    Clusters(ClusterId num_clusters, PointsSpace & ps) 
      : num_clusters_(num_clusters), ps_(ps), 
	num_dimensions_(ps.GetNumDimensions()),
	num_points_(ps.GetNumPoints()),
	points_to_clusters_(num_points_, 0){

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

    void run()
    {
        ClustersToPoints ctp(num_points_);
        for(uint32_t i=0;i<num_points_;i++)
        {
            ctp[i].insert(i);
        }
        std::vector<std::vector<double>> matrix(num_points_);
        for(uint32_t i=0;i<num_points_;i++)
        {
            std::vector<double>& vec = matrix[i];
            vec.resize(num_points_, -1.0);
            for(uint32_t j=i+1; j<num_points_;j++)
            {
                vec[j] = distance(ps_.GetPoint(i), ps_.GetPoint(j));
            }
        }
        while(true)
        {
            double min_dist = -1.0;
            std::pair<uint32_t, uint32_t> min_pair(0,0);
            for(uint32_t i=0;i<num_points_;i++)
            {
                if(ctp[i].empty()) continue;
                for(uint32_t j=i+1;j<num_points_;j++)
                {
                    if(ctp[j].empty()) continue;
                    double dist = matrix[i][j];
                    if(min_dist<0.0 || dist<min_dist)
                    {
                        min_dist = dist;
                        min_pair = std::make_pair(i,j);
                    }
                }
            }
            std::cerr<<"min dist "<<min_dist<<std::endl;
            if(min_dist<0.0) break;
            if(min_dist>15.0) break;
            uint32_t x = min_pair.first;
            uint32_t y = min_pair.second;
            //merge y to x
            ctp[x].insert(ctp[y].begin(), ctp[y].end());
            ctp[y].clear();
            for(uint32_t i=0;i<num_points_;i++)
            {
                if(ctp[i].empty()) continue;
                if(i==x) continue;
                double min_dist = -1.0;
                for(auto pi : ctp[i])
                {
                    for(auto px : ctp[x])
                    {
                        double dist = distance(ps_.GetPoint(pi), ps_.GetPoint(px));
                        if(min_dist<0.0 || dist<min_dist) min_dist = dist;
                    }
                }
                if(i>x) matrix[x][i] = min_dist;
                else matrix[i][x] = min_dist;
            }

        }
        for(auto points : ctp)
        {
            if(points.empty()) continue;
            clusters_to_points_.push_back(points);
        }
    }
    
    void k_means ()
    {
        bool move;
        bool some_point_is_moving = true;
        uint32_t num_iter = 0;
        PointId pid;
        ClusterId to_cluster;
        //
        // Initial partition of points
        //
        initial_partition_points();

        //
        // Until not converge
        //
        while (some_point_is_moving){

          //std::cerr<<"iter "<<num_iter<<std::endl;
          //for(ClusterId i=0;i<num_clusters_;i++)
          //{
          //  std::cerr<<"cid "<<i<<" : ";
          //  const SetPoints& sp = clusters_to_points_[i];
          //  for(auto p : sp) {
          //      std::cerr<<p<<",";
          //  }
          //  std::cerr<<std::endl;
          //}

          some_point_is_moving = false;

          compute_centroids();
          //      std::cout << "Centroids" << std::endl << centroids__;      

          //
          // for each point
          //
          for (pid=0; pid<num_points_; pid++){
        
            // distance from current cluster
            ClusterId cid = points_to_clusters_[pid];
            const SetPoints& sp = clusters_to_points_[cid];
            if(sp.size()<2) continue;
            Distance min = distance(centroids_[cid], ps_.GetPoint(pid));

            //std::cout << "pid[" << pid << "] in cluster=" 
            //      << points_to_clusters_[pid] 
            //      << " with distance=" << min << std::endl;

            //
            // foreach centroid
            //
            cid = 0; 
            move = false;
            BOOST_FOREACH(Centroids::value_type c, centroids_){
              

              Distance d = distance(c, ps_.GetPoint(pid));
              if (d < min){
                min = d;
                move = true;
                to_cluster = cid;

                // remove from current cluster
                clusters_to_points_[points_to_clusters_[pid]].erase(pid);

                some_point_is_moving = true;
                //std::cout << "\tcluster=" << cid 
                //      << " closer, dist=" << d << std::endl;	    
              }
              cid++;
            }
            
            //
            // move towards a closer centroid 
            //
            if (move){
              
              // insert
              points_to_clusters_[pid] = to_cluster;
              clusters_to_points_[to_cluster].insert(pid);
              //std::cout << "\t\tmove to cluster=" << to_cluster << std::endl;
            }
          }      

          num_iter++;
        } // end while (some_point_is_moving)

        //std::cout << std::endl << "Final clusters" << std::endl;
        //std::cout << clusters_to_points__;
    }
  };
}

#endif

