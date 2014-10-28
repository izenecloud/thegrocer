#ifndef THEGROCER_THEGROCER_H_ 
#define THEGROCER_THEGROCER_H_
#include <string>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/unordered_set.hpp>
#include <icma/icma.h>
#include "basis.hpp"
#include "kmeans.hpp"


namespace thegrocer {
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
    TheGrocer(uint32_t n=5):n_(n), knowledge_(NULL), analyzer_(NULL)
    {
    }

    ~TheGrocer()
    {
        if(analyzer_!=NULL) delete analyzer_;
        if(knowledge_!=NULL) delete knowledge_;
    }

    bool Load(const std::string& pcma, const std::string& resource)
    {
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
            wid_t wid = 0;
            word_list_.resize(1);
            while(getline(ifs, line))
            {
                boost::algorithm::trim(line);
                std::vector<std::string> vec;
                boost::algorithm::split(vec, line, boost::algorithm::is_any_of(" "));
                if(vec.size()<2) continue;
                ++wid;
                Word word;
                word.text = vec.front();
                for(uint32_t i=1;i<vec.size();i++)
                {
                    word.vec.push_back(atof(vec[i].c_str()));
                }
                text_map_[word.text] = wid;
                word_list_.push_back(word);
                dimension_ = word.vec.size();
            }
            ifs.close();
        }
        {
            std::ifstream ifs(products_file.c_str());
            std::string line;
            while(getline(ifs, line))
            {
                boost::algorithm::trim(line);
                if(line.length()<=32) continue;
                Product p;
                p.docid = line.substr(0, 32);
                p.title = line.substr(32);
                GetWords_(p.title, p.wid_vec);
                BagVector_(p.wid_vec, p.vec);
                pid_t pid = product_list_.size();
                for(uint32_t i=0;i<p.wid_vec.size();i++)
                {
                    word_list_[p.wid_vec[i]].posting.push_back(pid);
                }
                product_list_.push_back(p);
            }
            ifs.close();
        }
        if(word_list_.size()<10||product_list_.size()<10) return false;
        return true;
    }

    bool Query(const std::string& content, const std::string& title, std::vector<std::string>& result)
    {
        const static uint32_t K = 5u;
        WidVec title_wid_vec;
        GetWords_(title, title_wid_vec);
        vec_t title_vec;
        BagVector_(title_wid_vec, title_vec);
        WidVec query_wid_vec;
        GetWords_(content, query_wid_vec);
        PointsSpace ps(query_wid_vec.size(), dimension_);
        for(uint32_t i=0;i<ps.getNumPoints();i++)
        {
            Point& p = ps.getPoint(i);
            p = word_list_[query_wid_vec[i]].vec;
        }
        Clusters kmeans(K, ps);
        kmeans.k_means();
        const auto& ctp = kmeans.getClustersToPoints();
        PidSet pid_set;
        std::vector<vec_t> qvec_list;
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
            vec_t kvec;
            BagVector_(wv, kvec);
            double sim = cosine_(title_vec, kvec);
            //filter by sim
            qvec_list.push_back(kvec);
            for(auto i=0u;i<wv.size();i++)
            {
                const Word& word = word_list_[wv[i]];
                for(uint32_t j=0;j<word.posting.size();j++)
                {
                    pid_set.insert(word.posting[j]);
                }
            }
        }
        //calculate sim by qvec_list, with each pid in pid_set
        std::vector<std::pair<double, uint32_t> > ret;
        for(auto it = pid_set.begin();it!=pid_set.end();++it)
        {
            pid_t pid = *it;
            const vec_t& pvec = product_list_[pid].vec;
            double asim = 0.0;
            for(uint32_t i=0;i<qvec_list.size();i++)
            {
                asim += cosine_(pvec, qvec_list[i]);
            }
            ret.push_back(std::make_pair(asim, pid));
        }
        std::sort(ret.begin(), ret.end());
        boost::unordered_set<wid_t> wid_exist;
        
        for(auto it = ret.rbegin();it!=ret.rend();++it)
        {
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
            if((double)ecount/wid_vec.size()>=0.7)
            {
                continue;
            }
            for(uint32_t i=0;i<wid_vec.size();i++)
            {
                wid_exist.insert(wid_vec[i]);
            }
            const Product& product = product_list_[pid];
            std::string otitle = product.title;
            std::cout<<otitle<<std::endl;
            result.push_back(product.docid);
            if(result.size()==n_) break;
            //output pid
        }
        return true;
    }


private:
    void GetWords_(const std::string& text, WidVec& wid_vec)
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
            if(pos=="W" || wsize<2) continue;
            if(pos[0]!='N') continue;
            std::cout << str << "/"<<pos << " ";
            const auto& it = text_map_.find(str);
            if(it==text_map_.end()) continue;
            wid_t wid = it->second;
            if(exist.find(wid)!=exist.end()) continue;
            exist.insert(wid);
            wid_vec.push_back(wid);
            //std::cout << str <<" ";
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

    void BagVector_(const WidVec& wid_vec, vec_t& vec)
    {
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
    uint32_t n_;
    cma::Knowledge* knowledge_;
    cma::Analyzer* analyzer_;
    uint32_t dimension_;

    std::vector<Word> word_list_;
    TextMap text_map_;
    std::vector<Product> product_list_;
};

}

#endif

