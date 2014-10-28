#include <string>
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <icma/icma.h>
//#include <icma/me/CMA_ME_Knowledge.h>


namespace po = boost::program_options;
using namespace cma;
int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("wiki", po::value<std::string>(), "wiki extracted text file")
        ("cma,C", po::value<std::string>(), "cma knowledge path")
        ("test", "do test")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string cma;
    std::string wiki;
    if (vm.count("cma")) {
        cma = vm["cma"].as<std::string>();
        std::cerr << "cma: " << cma<<std::endl;
    } 
    if (vm.count("wiki")) {
        wiki = vm["wiki"].as<std::string>();
        std::cerr << "wiki: " << wiki<<std::endl;
    } 
    if(cma.empty()||wiki.empty())
    {
        std::cerr<<"parameters error"<<std::endl;
        return EXIT_FAILURE;
    }
    CMA_Factory* factory = CMA_Factory::instance();
    Analyzer* analyzer = factory->createAnalyzer();

    Knowledge* knowledge = factory->createKnowledge();
    analyzer->setOption(Analyzer::OPTION_TYPE_NBEST, 1);
    //const char* modelPath = "../db/icwb/utf8/";
    if(cma[cma.length()-1]!='/') cma+="/";
    std::size_t last = cma.find_last_of('/');
    std::size_t first = cma.find_last_of('/', last-1);
    std::string encode_str = cma.substr(first+1, last-first-1);
    //testFileSuffix = encodeStr;
    knowledge->loadModel(encode_str.data(), cma.c_str(), true);
    //std::size_t dic_size= ((CMA_ME_Knowledge*)knowledge)->getTrie()->size();
    //printf("[Info] All Dictionaries' Size: %.2fm.\n", dic_size/1048576.0);
    // set knowledge
    analyzer->setKnowledge(knowledge);
    analyzer->setOption( Analyzer::OPTION_ANALYSIS_TYPE, 1);

    std::ifstream ifs(wiki.c_str());
    std::string line;
    bool last_doc = false;
    setlocale( LC_ALL, "" );
    Sentence s;
    while(getline(ifs, line))
    {
        if(boost::algorithm::starts_with(line, "<doc"))
        {
            last_doc = true;
            std::cout<<std::endl;
            continue;
        }
        if(last_doc){
            last_doc = false;
            continue;
        }
        if(boost::algorithm::starts_with(line, "</doc"))
        {
            continue;
        }
        //boost::algorithm::trim(line);
    	if(line.size()<=1)
    		continue;
    	//if(boost::algorithm::starts_with(line, "田园诗派是中国古代诗歌的一个流派")) continue;
    	//if(boost::algorithm::starts_with(line, "日本江户时代")) break;
    	s.setString(line.c_str());

        int result = analyzer->runWithSentence(s);
        if(result != 1)
        {
            std::cerr << "fail in Analyzer::runWithSentence()" << std::endl;
            continue;
        }
        if(s.getListSize()!=1)
        {
            continue;
        }
        //std::cerr<<"[L]"<<line<<","<<s.getListSize()<<std::endl;
        int j = s.getOneBestIndex();
        //std::cerr<<"[B]"<<j<<std::endl;
        if(j<0) continue;
        int wc = s.getCount(j);
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
            std::cout << str << "/"<<pos << " ";
            //std::cout << str <<" ";
        }
    }
    ifs.close();
    //ofs.close();



    //clean in the end
    delete knowledge;
    delete analyzer;
    std::cerr<<"Finished, now you can run word2vec to generate the semantic vector for each word, example: word2vec -train <input-file> -output ./model"<<std::endl;
}

