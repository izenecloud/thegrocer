#ifndef THEGROCER_THEGROCER_H_ 
#define THEGROCER_THEGROCER_H_
#include <string>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/unordered_set.hpp>
#include <icma/icma.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/ml/ml.hpp>
#include "basis.hpp"
#include "clustering.hpp"
#include <thevoid/swarm/logger.hpp>



namespace thegrocer {
    using namespace blackhole;
class TheGrocer {
    typedef uint32_t wid_t;//word id
    typedef uint32_t pid_t;
    typedef std::vector<pid_t> posting_t;
    typedef std::vector<double> vec_t;
    typedef boost::unordered_map<std::string, wid_t> TextMap;
    typedef boost::unordered_set<wid_t> WidSet;
    typedef boost::unordered_set<pid_t> PidSet;
    typedef std::vector<wid_t> WidVec;
    struct Word {
        std::string text;
        vec_t vec;
        posting_t posting;
    };

    struct Product {
        std::string docid;
        std::string title;
        WidVec wid_vec;
        vec_t vec;
    };
public:
    TheGrocer(const izenecloud::swarm::logger& logger, uint32_t n=5):logger_(logger),n_(n), knowledge_(NULL), analyzer_(NULL), dimension_(200), nnk_(5)
    {
    }

    ~TheGrocer()
    {
        if(analyzer_!=NULL) delete analyzer_;
        if(knowledge_!=NULL) delete knowledge_;
    }

    bool Load(const std::string& pcma, const std::string& resource)
    {
        BH_LOG(logger_, SWARM_LOG_INFO, "start load resources");
        cma::CMA_Factory* factory = cma::CMA_Factory::instance();
        analyzer_ = factory->createAnalyzer();

        knowledge_ = factory->createKnowledge();
        analyzer_->setOption(cma::Analyzer::OPTION_TYPE_NBEST, 1);
        //const char* modelPath = "../db/icwb/utf8/";
        std::string cma(pcma);
        if(cma[cma.length()-1]!='/') cma+="/";
        std::size_t last = cma.find_last_of('/');
        std::size_t first = cma.find_last_of('/', last-1);
        std::string encode_str = cma.substr(first+1, last-first-1);
        //testFileSuffix = encodeStr;
        knowledge_->loadModel(encode_str.data(), cma.c_str(), true);
        //std::size_t dic_size= ((CMA_ME_Knowledge*)knowledge)->getTrie()->size();
        //printf("[Info] All Dictionaries' Size: %.2fm.\n", dic_size/1048576.0);
        // set knowledge
        analyzer_->setKnowledge(knowledge_);
        analyzer_->setOption( cma::Analyzer::OPTION_ANALYSIS_TYPE, 1);
        std::string model_file = resource+"/model";
        std::string products_file = resource+"/products";
        {
            std::ifstream ifs(model_file.c_str());
            std::string line;
            std::size_t p=0;
            while(getline(ifs, line))
            {
                p++;
                if(p%10000==0)
                {
                    std::cerr<<"loading model "<<p<<std::endl;
                }
                boost::algorithm::trim(line);
                std::vector<std::string> vec;
                boost::algorithm::split(vec, line, boost::algorithm::is_any_of(" "));
                if(vec.size()<2) continue;
                if(vec.size()!=dimension_+1){
                    std::cerr<<"error line:"<<line<<std::endl;
                    continue;
                }
                Word word;
                word.text = vec.front();
                for(uint32_t i=1;i<vec.size();i++)
                {
                    word.vec.push_back(atof(vec[i].c_str()));
                }
                wid_t wid = word_list_.size();
                text_map_[word.text] = wid;
                word_list_.push_back(word);
            }
            ifs.close();
            std::cerr<<"word list size "<<word_list_.size()<<std::endl;
            std::cerr<<"dimension "<<dimension_<<std::endl;
        }
        {
            std::size_t nrow = 1000000;
            //cv::Mat train(nrow, dimension_, CV_32FC1);
            //cv::Mat classes(nrow, 1, CV_32FC1);
            std::ifstream ifs(products_file.c_str());
            std::string line;
            while(getline(ifs, line))
            {
                boost::algorithm::trim(line);
                if(line.length()<=32) continue;
                pid_t pid = product_list_.size();
                Product product;
                product.docid = line.substr(0, 32);
                product.title = line.substr(32);
                GetWords_(product.title, product.wid_vec);
                BagVector_(product.wid_vec, product.vec);
                //vec_t pvec;
                //BagVector_(product.wid_vec, pvec);
                //uint32_t p = pid;
                //for(uint32_t i=0;i<dimension_;i++)
                //{
                //    train.at<float>(p, i) = (float)pvec[i];
                //}
                //classes.at<float>(p, 0) = (float)p;
                for(uint32_t i=0;i<product.wid_vec.size();i++)
                {
                    word_list_[product.wid_vec[i]].posting.push_back(pid);
                }
                product_list_.push_back(product);
                if(pid%100000==0)
                {
                    std::cerr<<"loading product "<<pid<<std::endl;
                }
                if(product_list_.size()==nrow) break;
            }
            ifs.close();
            std::cerr<<"product list size "<<product_list_.size()<<std::endl;
            //knn_ = new cv::KNearest(train, classes, cv::Mat(), false, nnk_);
            //std::cerr<<"knearest train finished"<<std::endl;
        }
        if(word_list_.size()<10||product_list_.size()<10) return false;
        BH_LOG(logger_, SWARM_LOG_INFO, "finished loading resources");
        return true;
    }

    bool Query(const std::string& content, const std::string& title, std::vector<std::pair<std::string, std::string> >& result)
    {
        BH_LOG(logger_, SWARM_LOG_DEBUG, "start query");
        WidVec title_wid_vec;
        GetWords_(title, title_wid_vec);
        vec_t title_vec;
        BagVector_(title_wid_vec, title_vec);
        WidVec query_wid_vec;
        GetWords_(content, query_wid_vec, false, true);
        for(uint32_t i=0;i<query_wid_vec.size();i++)
        {
            wid_t wid = query_wid_vec[i];
            Word& word = word_list_[wid];
            BH_LOG(logger_, SWARM_LOG_DEBUG, "[W] %s", word.text.c_str());
        }
        BH_LOG(logger_, SWARM_LOG_DEBUG, "end analyzer");
        //for(uint32_t i=0;i<query_wid_vec.size();i++)
        //{
        //    for(uint32_t j=i+1;j<query_wid_vec.size();j++)
        //    {
        //        double dist = distance_(word_list_[query_wid_vec[i]].vec, word_list_[query_wid_vec[j]].vec);
        //        std::cerr<<"[DIST-"<<i<<","<<j<<"]"<<dist<<std::endl;
        //    }
        //}
        uint32_t K = 5;
        uint32_t PK = 2;
        uint32_t SK = n_*10;
        ClustersToPoints ctp;
        if(query_wid_vec.size()>K)
        {
            PointsSpace ps(query_wid_vec.size(), dimension_);
            for(uint32_t i=0;i<ps.GetNumPoints();i++)
            {
                Point& p = ps.GetPoint(i);
                p = word_list_[query_wid_vec[i]].vec;
            }
            Clusters clustering(K, ps);
            //clustering.run();
            clustering.k_means();
            BH_LOG(logger_, SWARM_LOG_DEBUG, "end clustering");
            ctp = clustering.GetClustersToPoints();
        }
        else {
            ctp.resize(query_wid_vec.size());
            for(uint32_t i=0;i<query_wid_vec.size();i++)
            {
                ctp[i].insert(i);
            }
            PK = ctp.size();
        }
        PidSet pid_set;
        std::vector<std::pair<double, vec_t>> qvec_list;
        WidVec candidate_wid_list;
        for(uint32_t cid=0;cid<ctp.size();cid++)
        {
            const auto& setpoints = ctp[cid];
            WidVec wv;
            wv.reserve(setpoints.size());
            for(auto it = setpoints.begin();it!=setpoints.end();++it)
            {
                auto wid = query_wid_vec[*it];
                wv.push_back(wid);
            }
            //for(uint32_t i=0;i<wv.size();i++)
            //{
            //    wid_t wid = wv[i];
            //    std::cerr<<"[C"<<cid<<"]"<<word_list_[wid].text<<std::endl;
            //}
            vec_t kvec;
            BagVector_(wv, kvec);
            double sim = -1.0;
            double dist = -1.0;
            if(!title_vec.empty()&&query_wid_vec.size()>K)//has title parameter
            {
                sim = cosine_(title_vec, kvec);
                //for(wid_t wid : wv) {
                //    std::cerr<<word_list_[wid].text<<",";
                //}
                //std::cerr<<"\tk sim "<<sim<<std::endl;
                if(sim<0.33) continue;
                dist = 1.0-sim;
            }
            candidate_wid_list.insert(candidate_wid_list.end(), wv.begin(), wv.end());
            //filter by sim
            qvec_list.push_back(std::make_pair(dist, kvec));
            for(auto i=0u;i<wv.size();i++)
            {
                const Word& word = word_list_[wv[i]];
                for(uint32_t j=0;j<word.posting.size();j++)
                {
                    pid_set.insert(word.posting[j]);
                }
            }
        }
        std::sort(qvec_list.begin(), qvec_list.end());
        BH_LOG(logger_, SWARM_LOG_DEBUG, "end pid selection %d", pid_set.size());
        if(qvec_list.size()>PK&&pid_set.size()>SK) {
            std::vector<std::pair<double, uint32_t> > tret;
            for(auto it = pid_set.begin();it!=pid_set.end();++it)
            {
                pid_t pid = *it;
                const Product& product = product_list_[pid];
                const vec_t& pvec = product.vec;
                double dist = 0.0;
                for(uint32_t i=0;i<PK;i++)
                {
                    dist += distance_(pvec, qvec_list[i].second);
                }
                dist *= std::log(4.0+product.wid_vec.size());
                tret.push_back(std::make_pair(dist, pid));
            }
            std::sort(tret.begin(), tret.end());
            for(uint32_t i=SK;i<tret.size();i++)
            {
                pid_set.erase(tret[i].second);
            }
            BH_LOG(logger_, SWARM_LOG_DEBUG, "cleared pid size %d", pid_set.size());
        }
        //calculate sim by qvec_list, with each pid in pid_set
        //uint32_t nrow = pid_set.size();
        //cv::Mat train(nrow, dimension_, CV_32FC1);
        //cv::Mat classes(nrow, 1, CV_32FC1);
        //std::vector<pid_t> pid_vec(pid_set.begin(), pid_set.end());
        //for(uint32_t p=0;p<pid_vec.size();p++)
        //{
        //    pid_t pid = pid_vec[p];
        //    const Product& product = product_list_[pid];
        //    const vec_t& pvec = product.vec;
        //    for(uint32_t i=0;i<dimension_;i++)
        //    {
        //        train.at<float>(p, i) = (float)pvec[i];
        //    }
        //    classes.at<float>(p, 0) = (float)p;
        //}
        //cv::KNearest knn(train, classes, cv::Mat(), false, nnk_);
        //cv::Mat mat_test(qvec_list.size(), dimension_, CV_32FC1);
        //for(uint32_t i=0;i<qvec_list.size();i++)
        //{
        //    for(uint32_t d=0;d<dimension_;d++)
        //    {
        //        mat_test.at<float>(i, d) = qvec_list[i][d];
        //    }
        //}
        //cv::Mat responses(qvec_list.size(), nnk_, CV_32FC1);
        //cv::Mat dists(qvec_list.size(), nnk_, CV_32FC1);
        //knn.find_nearest(mat_test, nnk_, 0,0, &responses, &dists);
        std::vector<std::pair<double, uint32_t> > ret;
        for(auto it = pid_set.begin();it!=pid_set.end();++it)
        {
            pid_t pid = *it;
            const Product& product = product_list_[pid];
            const vec_t& pvec = product.vec;
            double dist = 0.0;
            for(uint32_t i=0;i<qvec_list.size();i++)
            {
                dist += distance_(pvec, qvec_list[i].second);
            }
            dist *= std::log(4.0+product.wid_vec.size());
            ret.push_back(std::make_pair(dist, pid));
        }


        std::sort(ret.begin(), ret.end());
        BH_LOG(logger_, SWARM_LOG_DEBUG, "end ret computation");
        boost::unordered_set<wid_t> wid_exist;
        
        for(auto it = ret.begin();it!=ret.end();++it)
        {
            double score = it->first;
            pid_t pid = it->second;
            const WidVec& wid_vec = product_list_[pid].wid_vec;
            uint32_t ecount=0;
            for(uint32_t i=0;i<wid_vec.size();i++)
            {
                if(wid_exist.find(wid_vec[i])!=wid_exist.end())
                {
                    ecount++;
                }
            }
            const Product& product = product_list_[pid];
            std::string otitle = product.title;
            if((double)ecount/wid_vec.size()>=0.8)
            {
                BH_LOG(logger_, SWARM_LOG_DEBUG, "p ignoring %s", otitle.c_str());
                continue;
            }
            for(uint32_t i=0;i<wid_vec.size();i++)
            {
                wid_exist.insert(wid_vec[i]);
            }
            std::pair<double, wid_t> select(-1.0, 0);
            for(wid_t wid : candidate_wid_list) {
                double dist = distance_(word_list_[wid].vec, product.vec);
                if(select.first<0.0 || dist<select.first)
                {
                    select.first = dist;
                    select.second = wid;
                }
            }
            const std::string& keyword = word_list_[select.second].text;
            BH_LOG(logger_, SWARM_LOG_DEBUG, "[P]%s,%f,%s", otitle.c_str(), score, keyword.c_str());
            result.push_back(std::make_pair(product.docid, keyword));
            if(result.size()==n_) break;
            //output pid
        }
        BH_LOG(logger_, SWARM_LOG_DEBUG, "end all");
        return true;
    }


private:
    void GetWords_(const std::string& text, WidVec& wid_vec, bool all=true, bool debug=false)
    {
        cma::Sentence s;
    	s.setString(text.c_str());
        int result = analyzer_->runWithSentence(s);
        if(result != 1)
        {
            std::cerr << "fail in Analyzer::runWithSentence()" << std::endl;
            return;
        }
        if(s.getListSize()!=1)
        {
            return;
        }
        //std::cerr<<"[L]"<<line<<","<<s.getListSize()<<std::endl;
        int j = s.getOneBestIndex();
        //std::cerr<<"[B]"<<j<<std::endl;
        if(j<0)
        {
            return;
        }
        int wc = s.getCount(j);
        boost::unordered_set<wid_t> exist;
        for(int i=0;i<wc;i++)
        {
            const char* word = s.getLexicon(0, i);
            std::string str(word);
            boost::to_lower(str);
            boost::algorithm::replace_all(str, " ", "");
            std::string pos(s.getStrPOS(0, i));
            std::size_t csize = str.length();
            //std::wstring wstr(csize, L'#');
            std::size_t wsize = mbstowcs(NULL, word, csize);
            if(debug)
                std::cerr<<str<<"/"<<pos<<"/"<<wsize<<" ";
            if(pos=="W" || wsize<2) continue;
            if(pos[0]!='N') continue;
            if(!all)
            {
                if(wsize==2&&pos=="N") continue;
            }
            auto it = text_map_.find(str);
            if(it==text_map_.end()) continue;
            wid_t wid = it->second;
            if(exist.find(wid)!=exist.end()) continue;
            exist.insert(wid);
            wid_vec.push_back(wid);
            //std::cout << str <<" ";
        }
        if(debug)
        {
            std::cerr<<std::endl;
        }
    }
    static double cosine_(const vec_t& v1, const vec_t& v2)
    {
        double a = 0.0;
        double b = 0.0;
        double c = 0.0;
        for(auto i=0u;i<v1.size();i++)
        {
            a+=v1[i]*v1[i];
            b+=v2[i]*v2[i];
            c+=v1[i]*v2[i];
        }
        return c/(sqrt(a)*sqrt(b));
    }

    static double distance_(const vec_t& v1, const vec_t& v2)
    {
        double total = 0.0;
        for(uint32_t i=0;i<v1.size();i++)
        {
            double diff = v1[i]-v2[i];
            total += diff*diff;
        }
        return total;
    }

    void BagVector_(const WidVec& wid_vec, vec_t& vec)
    {
        if(wid_vec.empty()) return;
        vec.resize(dimension_, 0.0);
        for(auto it = wid_vec.begin();it!=wid_vec.end();++it)
        {
            const auto& v = word_list_[*it].vec;
            for(auto i=0u;i<vec.size();i++)
            {
                vec[i]+=v[i];
            }
        }
    }


private:
    const izenecloud::swarm::logger& logger_;
    uint32_t n_;
    cma::Knowledge* knowledge_;
    cma::Analyzer* analyzer_;
    uint32_t dimension_;

    std::vector<Word> word_list_;
    TextMap text_map_;
    std::vector<Product> product_list_;
    //cv::KNearest* knn_;
    uint32_t nnk_;
};

}

#endif

