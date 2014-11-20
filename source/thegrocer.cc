#include <boost/program_options.hpp>
#include <thegrocer/thegrocer.hpp>
#include <thevoid/swarm/logger.hpp>
#include <thevoid/server.hpp>
#include <thevoid/stream.hpp>
#include <thevoid/rapidjson/rapidjson.h>
#include <thevoid/rapidjson/stringbuffer.h>
#include <thevoid/rapidjson/writer.h>

namespace po = boost::program_options;
using namespace izenecloud;
using namespace thegrocer;

static TheGrocer* ptg = NULL;

class http_server : public thevoid::server<http_server>
{
public:
	virtual bool initialize(const rapidjson::Value &config) {
		(void) config;

		on<on_ping>(
			options::exact_match("/ping"),
			options::methods("GET")
		);
		on<on_timeout>(
			options::exact_match("/timeout"),
			options::methods("GET")
		);
		on<on_get>(
			options::exact_match("/get"),
			options::methods("GET")
		);
		on<on_post>(
			options::exact_match("/post"),
			options::methods("POST")
		);
		on<on_echo>(
			options::exact_match("/echo"),
			options::methods("GET")
		);
		on<on_ping>(
			options::exact_match("/header-check"),
			options::methods("GET"),
			options::header("X-CHECK", "SecretKey")
		);
	
		return true;
	}

	struct on_ping : public thevoid::simple_request_stream<http_server> {
		virtual void on_request(const thevoid::http_request &req, const boost::asio::const_buffer &buffer) {
			(void) buffer;
			(void) req;

			this->send_reply(thevoid::http_response::ok);
		}
	};

	struct on_timeout : public thevoid::simple_request_stream<http_server> {
		virtual void on_request(const thevoid::http_request &req, const boost::asio::const_buffer &buffer) {
			(void) buffer;
			(void) req;
			if (auto timeout = req.url().query().item_value("timeout")) {
				BH_LOG(logger(), SWARM_LOG_INFO, "timeout: %s", timeout->c_str());
				usleep(atoi(timeout->c_str()) * 1000);
			}

			this->send_reply(thevoid::http_response::ok);
		}
	};

	struct on_get : public thevoid::simple_request_stream<http_server> {
		virtual void on_request(const thevoid::http_request &req, const boost::asio::const_buffer &buffer) {
			(void) buffer;

            std::string title;
            std::string content;
			if (auto datap = req.url().query().item_value("title"))
				title= *datap;
			if (auto datap = req.url().query().item_value("content"))
				content = *datap;

            std::vector<std::pair<std::string, std::string> > results;
			ptg->Query(content, title, results);
            rapidjson::StringBuffer s;
            rapidjson::Writer<rapidjson::StringBuffer> writer(s);
            writer.StartArray();
            for(uint32_t i=0;i<results.size();i++)
            {
                writer.StartArray();
                writer.String(results[i].first.c_str());
                writer.String(results[i].second.c_str());
                writer.EndArray();
            }
            writer.EndArray();
            std::string res = s.GetString();
			thevoid::http_response reply;
			reply.set_code(thevoid::http_response::ok);
			reply.headers().set_content_length(res.size());

			this->send_reply(std::move(reply), std::move(res));
		}
	};

	struct on_post : public thevoid::simple_request_stream<http_server> {
		virtual void on_request(const thevoid::http_request &req, const boost::asio::const_buffer &buffer) {
			(void) buffer;

			std::string data;
            std::string title;
            std::string content;
			if (auto datap = req.url().query().item_value("data"))
				data = *datap;
			if (auto datap = req.url().query().item_value("title"))
				title= *datap;
			if (auto datap = req.url().query().item_value("content"))
				content = *datap;
		    std::cerr<<"query title "<<title<<std::endl;
		    std::cerr<<"query content "<<content<<std::endl;

			int timeout_ms = 10 + (rand() % 10);
			usleep(timeout_ms * 1000);

			thevoid::http_response reply;
			reply.set_code(thevoid::http_response::ok);
			reply.headers().set_content_length(data.size());
            //std::vector<std::string> results;
			//ptg->Query(content, title, results);

			this->send_reply(std::move(reply), std::move(data));
		}
	};

	struct on_echo : public thevoid::simple_request_stream<http_server> {
		virtual void on_request(const thevoid::http_request &req, const boost::asio::const_buffer &buffer) {
			auto data = boost::asio::buffer_cast<const char*>(buffer);
			auto size = boost::asio::buffer_size(buffer);

			thevoid::http_response reply;
			reply.set_code(thevoid::http_response::ok);
			reply.set_headers(req.headers());
			reply.headers().set_content_length(size);

			this->send_reply(std::move(reply), std::string(data, size));
		}
	};
};


int main(int ac, char** av)
{
    setlocale( LC_ALL, "" );
    //std::string str = "孤岛惊魂";
    //std::size_t csize = str.length();
    //std::size_t wsize = mbstowcs(NULL, str.c_str(), csize);
    //std::cerr<<"wsize "<<wsize<<std::endl;
    //return EXIT_SUCCESS;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("knowledge", po::value<std::string>(), "knowledge resource dir")
        ("cma,C", po::value<std::string>(), "cma knowledge path")
        ("config", po::value<std::string>(), "config file path")
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
    std::string config;
    if (vm.count("cma")) {
        cma = vm["cma"].as<std::string>();
        std::cerr << "cma: " << cma<<std::endl;
    } 
    if (vm.count("knowledge")) {
        knowledge = vm["knowledge"].as<std::string>();
        std::cerr << "knowledge: " << knowledge<<std::endl;
    } 
    if (vm.count("config")) {
        config = vm["config"].as<std::string>();
        std::cerr << "config: " << config<<std::endl;
    } 
    if(cma.empty()||knowledge.empty())
    {
        std::cerr<<"parameters error"<<std::endl;
        return EXIT_FAILURE;
    }
    int argc = 3;
    char** argv = new char*[argc];
    std::string cname = "--config";
    argv[0] = av[0];
    argv[1] = (char*)cname.c_str();
    argv[2] = (char*)config.c_str();
    std::shared_ptr<http_server> server = thevoid::create_server<http_server>();
    server->parse_arguments(argc, argv);
    const auto& logger = server->logger();
    //BH_LOG(logger, blackhole::defaults::severity::warning, "aaaaa");
    ptg = new TheGrocer(logger, 5);
    //TheGrocer tg(5);
    if(!ptg->Load(cma, knowledge))
    {
        std::cerr<<"load error"<<std::endl;
        return EXIT_FAILURE;
    }
    //std::vector<std::string> result;
    //tg.Query("网易数码 10月29日消息，苹果CEO蒂姆·库克（Tim Cook）iPhone 5s在参加《华尔街日报》主办的WSJD Live会议上对智能手表Apple Watch和移动支付平台Apple Pay发表了一些自己的看法，其中包括大家都比较关注的Apple Watch的电池续航时间。《华尔街日报》的主编杰拉德·贝克（Gerard Baker）向库克询问了关于Apple Watch的电池续航时间的问题，当贝克问用户是否能在夜间正常使用Apple Watch，库克回答称由于用户未来使用Apple Watch的频率比较高，所以其续航时间将不会太长，而用户则可能需要一天一充。同时库克还表示，苹果还在研究用户未来会如何使用Apple Watch，“我想大家应该会适应每天都为你的Apple Watch进行充电的，”库克在最后说道。实际上库克在这次回答中并未给出Apple Watch的电池续航时间，所以这个问题的答案可能只有等到这款智能手表到明年年初上市时才能揭晓了。（半缘）","库克谈Apple Watch电池续航：需一天一充",result);
    return server->run();
	//return thevoid::run_server<http_server>(argc, argv);
}


