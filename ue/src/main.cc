/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2015 The srsUE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsUE library.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include "ue.h"

//TODO: Get version from cmake
#define VERSION "0.1.0"

using namespace std;
namespace bpo = boost::program_options;

void sig_int_handler(int signo)
{
  printf("bye\n");
  exit(0);
}

/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
string config_file;

void parse_args(srsue::all_args_t *args, int argc, char* argv[]) {

    // Command line only options
    bpo::options_description general("General options");
    general.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Print version information and exit")
        ;

    // Command line or config file options
    bpo::options_description common("Configuration options");
    common.add_options()
        ("rf.dl_freq",        bpo::value<float>(&args->rf.dl_freq),       "Downlink centre frequency")
        ("rf.ul_freq",        bpo::value<float>(&args->rf.ul_freq),       "Uplink centre frequency")
        ("rf.rx_gain",        bpo::value<float>(&args->rf.rx_gain),       "Front-end receiver gain")
        ("rf.tx_gain",        bpo::value<float>(&args->rf.tx_gain),       "Front-end transmitter gain")

        ("pcap.enable",                                                   "Enable MAC packet captures for wireshark")
        ("pcap.filename",     bpo::value<string>(&args->pcap.filename),   "MAC layer capture filename")

        ("log.phy_level",     bpo::value<string>(&args->log.phy_level),   "PHY log level")
        ("log.phy_hex_limit", bpo::value<int>(&args->log.phy_hex_limit),  "PHY log hex dump limit")
        ("log.mac_level",     bpo::value<string>(&args->log.mac_level),   "MAC log level")
        ("log.mac_hex_limit", bpo::value<int>(&args->log.mac_hex_limit),  "MAC log hex dump limit")
        ("log.rlc_level",     bpo::value<string>(&args->log.rlc_level),   "RLC log level")
        ("log.rlc_hex_limit", bpo::value<int>(&args->log.rlc_hex_limit),  "RLC log hex dump limit")
        ("log.pdcp_level",    bpo::value<string>(&args->log.pdcp_level),  "PDCP log level")
        ("log.pdcp_hex_limit",bpo::value<int>(&args->log.pdcp_hex_limit), "PDCP log hex dump limit")
        ("log.rrc_level",     bpo::value<string>(&args->log.rrc_level),   "RRC log level")
        ("log.rrc_hex_limit", bpo::value<int>(&args->log.rrc_hex_limit),  "RRC log hex dump limit")
        ("log.gw_level",      bpo::value<string>(&args->log.gw_level),    "GW log level")
        ("log.gw_hex_limit",  bpo::value<int>(&args->log.gw_hex_limit),   "GW log hex dump limit")
        ("log.nas_level",     bpo::value<string>(&args->log.nas_level),   "NAS log level")
        ("log.nas_hex_limit", bpo::value<int>(&args->log.nas_hex_limit),  "NAS log hex dump limit")
        ("log.all_level",     bpo::value<string>(&args->log.all_level),   "ALL log level")
        ("log.all_hex_limit", bpo::value<int>(&args->log.all_hex_limit),  "ALL log hex dump limit")
    ;

    // Positional options - config file location
    bpo::options_description position("Positional options");
    position.add_options()
    ("config_file", bpo::value< string >(&config_file), "UE configuration file")
    ;
    bpo::positional_options_description p;
    p.add("config_file", -1);

    // these options are allowed on the command line
    bpo::options_description cmdline_options;
    cmdline_options.add(common).add(position).add(general);

    // parse the command line and store result in vm
    bpo::variables_map vm;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    bpo::notify(vm);

    // help option was given - print usage and exit
    if (vm.count("help")) {
        cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl << endl;
        cout << common << endl << general << endl;
        exit(0);
    }

    // print version number and exit
    if (vm.count("version")) {
        cout << "Version " << VERSION << endl;
        exit(0);
    }

    // no config file given - print usage and exit
    if (!vm.count("config_file")) {
        cout << "Error: Configuration file not provided" << endl;
        cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl << endl;
        exit(0);
    } else {
        cout << "Reading configuration file " << config_file << "..." << endl;
        ifstream conf(config_file.c_str(), ios::in);
        if(conf.fail()) {
          cout << "Failed to read configuration file " << config_file << " - exiting" << endl;
          exit(1);
        }
        bpo::store(bpo::parse_config_file(conf, common), vm);
        bpo::notify(vm);
    }

    if (vm.count("enable_pcap")) {
      args->pcap.enable = true;
    }
}


int main(int argc, char *argv[]) {

  signal(SIGINT, sig_int_handler);
  srsue::all_args_t args;

  cout << "---  Software Radio Systems LTE UE  ---" << endl << endl;
  parse_args(&args, argc, argv);
  while(1) {
    usleep(100000);
  }
}
