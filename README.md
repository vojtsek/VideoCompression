README
======
Program for distributed encoding of video files.
It was created as a school project.
Faculty of Mathematics and Physics, Charles University in Prague
Author Vojtech Hudecek 2015

The source code is available at

https://github.com/vojtsek/VideoCompression.git


INSTALLATION
------------
To install please run script install.sh in the root folder and follow the instructions.
The script generates configuration file and automatically builds the executable
in the bin/ directory.

CONFIGURATION
-------------
The configuration is saved in the bin/CONF file, which is created by the installation script.
It's essential to provide valid path to the ffmpeg and ffprobe executable and
adress with port of the neighbor that should be contacted initially.
The field MY_IP is not essential as long as the initial neighbor is alive - it will be recognized automatically.

Some of the settings can be changed by providing options.
 * -s ... no address will be contacted initially
 * -n address~port_number ...node to contact initially
 * -a address~port_number ...address and port to bound to
 * -h directory ...path to the home directory
 * -i file ...file to encode
 * -p port ...listenning port
 * -d level ...debug level
 * -q quality ...quality of encoding

If the string *ipv4* appears among parameters, the program will use only the IPv4 addresses,
in which case should be the *CONF* file changed appropriately.

SAMPLE USE
----------
After the program starts and joins the network, list of neighbors should be shown.
The following keys are used to control the program:
 * **F6** choose what information to show
 * **F7** start the process
 * **F8** load the file
 * **F9** set some parameters
 * **F10** abort the process
 * **F12** exit the program
Some of the actions provides options, which can be switched using **UP** and **DOWN** arrow keys.

The file needs to be loaded first and then the process can starts.
The result file can be found in the working directory, in the folder named as the listening port.
