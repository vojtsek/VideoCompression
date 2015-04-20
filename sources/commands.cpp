#include "headers/commands.h"
#include "headers/include_list.h"
#include "headers/handle_IO.h"
#include "headers/defines.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <map>
#include <utility>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace std;
using namespace utilities;

void Command::execute() {
    cursToInfo();
}
