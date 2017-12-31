#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <vector>
#include <sstream>
#include <iterator>

using json = nlohmann::json;
using namespace std;
template<typename Out>
void split(const std::string &s, char delim, Out result) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

namespace ns {
	struct algoritm {
		std::string algo;
		std::string current_mining_coin;
		std::string host;
		std::string all_host_list;
		std::vector<std::string> hosts;
		int port;
		int algo_switch_port;
		int multialgo_switch_port;
		double profit;
		double normalized_profit_amd;
		double normalized_profit_nvidia;
	};
	struct answer {
		bool success;
		json ret;
	};
	bool maxNvidiaProfit(const algoritm& rhs, const algoritm& lhs){
		return lhs.normalized_profit_nvidia < rhs.normalized_profit_nvidia;
	}

	void to_json(json& j, const algoritm& p){
		j = json{{"algo", p.algo},
			{"current_mining_coin", p.current_mining_coin},
			{"host", p.host},
			{"all_host_list", p.all_host_list},
			{"port", p.port},
			{"algo_switch_port", p.algo_switch_port},
			{"profit", p.profit},
			{"normalized_profit_nvidia", p.normalized_profit_nvidia},
			{"normalized_profit_amd", p.normalized_profit_amd}
		};
	}
	void from_json(const json& j, algoritm& p){
		p.algo = j.at("algo").get<std::string>();
		p.current_mining_coin = j.at("current_mining_coin").get<std::string>();
		p.host = j.at("host").get<std::string>();
		p.all_host_list = j["all_host_list"].get<std::string>();
		p.hosts = split(j["all_host_list"].get<std::string>(), ';');
		p.port = j.at("port").get<int>();
		p.algo_switch_port = j.at("algo_switch_port").get<int>();
		p.profit = j.at("profit").get<double>();
		p.normalized_profit_nvidia = j.at("normalized_profit_nvidia").get<double>();
		p.normalized_profit_amd = j.at("normalized_profit_amd").get<double>();
	}
}


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void startMining(ns::algoritm a){
	std::string host;
	std::string loc = "europe";
	for(int i = 0; i < a.hosts.size(); i++){
		if(a.hosts[i].find(loc) != std::string::npos){
			host = a.hosts[i];
		}
	}
	if(a.hosts.size() == 1){
		host = a.host;
	}
	std::cout << "starting to mine " << a.algo << " on port: " << a.algo_switch_port << std::endl;
	std::string executeString = "ccminer -a " + a.algo + " -o stratum+tcp://";
	executeString += host + ":";
	executeString += std::to_string(a.algo_switch_port);
	executeString += " -u jlowzow.ygdrasil -p x";
	std::transform(executeString.begin(), executeString.end(), executeString.begin(), ::tolower);
	std::cout << executeString << std::endl;

	const char * exe = executeString.c_str();

	pid_t pid;
	int status, died;
	switch(pid=fork()){
		case -1: cout << "can't fork\n";
				 exit(-1);
		case 0: execl(exe, "ccminer", 0);
		default: died = wait(&status);
				 cout << "finish" << endl;
	}
}




int main(void)
{
	json j;
	CURL *curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://miningpoolhub.com/index.php?page=api&action=getautoswitchingandprofitsstatistics");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		j = json::parse(readBuffer);
	}

	ns::answer ans {
		j["success"].get<bool>(),
		j["return"].get<json>()
	};

	std::vector<json> v = j["return"].get<std::vector<json>>();
	std::vector<ns::algoritm> liste;

	for(int i=0; i < v.size(); i++){
		liste.push_back(v[i]);
	}

	std::sort(liste.begin(), liste.end(), ns::maxNvidiaProfit);	

	for (int i=0; i < liste.size(); i++){
		std::cout << liste[i].algo<< " "<< liste[i].normalized_profit_nvidia << std::endl;
	}

	startMining(liste[0]);

	return 0;
}
