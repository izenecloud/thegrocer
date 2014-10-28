#include <boost/program_options.hpp>
#include <thegrocer/thegrocer.hpp>


namespace po = boost::program_options;
using namespace thegrocer;
int main(int ac, char** av)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("knowledge", po::value<std::string>(), "knowledge resource dir")
        ("cma,C", po::value<std::string>(), "cma knowledge path")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    std::string cma;
    std::string knowledge;
    if (vm.count("cma")) {
        cma = vm["cma"].as<std::string>();
        std::cerr << "cma: " << cma<<std::endl;
    } 
    if (vm.count("knowledge")) {
        knowledge = vm["knowledge"].as<std::string>();
        std::cerr << "knowledge: " << knowledge<<std::endl;
    } 
    if(cma.empty()||knowledge.empty())
    {
        std::cerr<<"parameters error"<<std::endl;
        return EXIT_FAILURE;
    }
    TheGrocer tg(5);
    if(!tg.Load(cma, knowledge))
    {
        std::cerr<<"load error"<<std::endl;
        return EXIT_FAILURE;
    }
    std::vector<std::string> result;
    tg.Query("","",result);
}


