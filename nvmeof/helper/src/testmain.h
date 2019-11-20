#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cstring>
#include "ns3/SSD_Defs.h"
#include "ns3/Execution_Parameter_Set.h"
#include "ns3/SSD_Device.h"
#include "ns3/Host_System.h"
#include "ns3/rapidxml.hpp"
#include "ns3/DistributionTypes.h"

using namespace std;


void command_line_args(const char* argv[], string& input_file_path, string& workload_file_path);


void read_configuration_parameters(const string ssd_config_file_path, Execution_Parameter_Set* exec_params);

std::vector<std::vector<IO_Flow_Parameter_Set*>*>* read_workload_definitions(const string workload_defs_file_path);

void collect_results(SSD_Device& ssd, Host_System& host, const char* output_file_path);

void print_help();

int testmain();
