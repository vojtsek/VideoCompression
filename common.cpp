#include "../headers/common.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int common::readCmd(istream &ins){
	string line, cmd_name;
	getline(ins, line);
	stringstream ss(line);
	ss >> cmd_name;
	if (cmd_name == "Quit")
		return (1);
	
	return (0);
}