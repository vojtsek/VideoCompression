#! /bin/bash

export CONFIG_FILE="/tmp/p2p.conf"
WORKING_DIR=${0%/*}
export BUILD_DIR="${WORKING_DIR}/bin"

function check_executable {
	[[ -x "$1" ]] || return 1
	return 0
}

# get_response question default
function get_response {
	echo -n "$1 (empty for '$2'):"
	read response
	[[ -z "$response" ]] && echo -n "$2" > response.tmp && return
	echo -n "$response" > response.tmp
}

function gen_conf_row {
	echo "# $1" >> "$CONFIG_FILE"
	echo "$2 $3" >> "$CONFIG_FILE"
}

function cleanup {
	rm "$CONFIG_FILE"
	rm  response.tmp
	rm *.o
	rm Makefile
}

function exit_config {
	echo $1
	cleanup
	exit 1
}

echo "Generated configuration" > "$CONFIG_FILE"
date >> "$CONFIG_FILE"

WD="$HOME"
FFM=$(which ffmpeg)
FFP=$(which ffprobe)
TTL=2
CHSIZE=5000
ACCEPTED=2
CTIME=60
TRIES=2
NCHECK=30
ENOUGH=2
LPORT=2222
CPORT=6666
CADDR="::ffff:127.0.0.1"
IP="::ffff:"$(ip addr | sed -n 's/.*inet \([0-9.]*\)\/.*$/\1/p' | grep -v 127.0.0.1)
QUALITY=fast

get_response "Set configuration?" "yes"
if [[ `head -n1 response.tmp` == "yes" ]]; then
	get_response "Path to working directory" "$WD"
	WD=`cat response.tmp`
	if [[ ! -d "$WD" ]]; then
		echo "Creating directory '$WD'"
		mkdir -p "$WD" || exit_config "Failed to create: '$WD'"
	fi

	get_response "Path to ffmpeg executable" "$FFM"
	FFM=`cat response.tmp`
	[[ -x "$FFM" ]] || exit_config "Not valid executable '$FFM'"

	get_response "Path to ffprobe executable" "$FFP"
	FFP=`cat response.tmp`
	[[ -x "$FFM" ]] || exit_config "Not valid executable '$FFP'"
	
	get_response "Time to live of advertising packets [seconds]" "$TTL"
	TTL=`cat response.tmp`

	get_response "Chunk size [kB]" "$CHSIZE"
	CHSIZE=`cat response.tmp`

	get_response "Number of accepted chunks in parallel" "$ACCEPTED"
	ACCEPTED=`cat response.tmp`

	get_response "Timeout before check chunk [seconds]" "$CTIME"
	CTIME=`cat response.tmp`

	get_response "How many retries for each chunk before resend" "$TRIES"
	TRIES=`cat response.tmp`

	get_response "Neighbor check interval [seconds]" "$NCHECK"
	NCHECK=`cat response.tmp`

	get_response "How many neighbors is enough?" "$ENOUGH"
	ENOUGH=`cat response.tmp`

	get_response "On which port listen" "$LPORT"
	LPORT=`cat response.tmp`

	get_response "On which port contact default" "$CPORT"
	CPORT=`cat response.tmp`

	get_response "Which address contact by default?" "$CADDR"
	CADDR=`cat response.tmp`

	get_response "IP address used" "$IP"
	IP=`cat response.tmp`

	get_response "What quality of encoding use?" "$QUALITY"
	QUALITY=`cat response.tmp`
fi

gen_conf_row "path to working directory" "WD" "$WD"
gen_conf_row "Path to ffmpeg executable" "FFMPEG_LOCATION" "$FFM"
gen_conf_row "Path to ffprobe executable" "FFPROBE_LOCATION" "$FFP"
gen_conf_row "Time to live of advertising packets [seconds]" "TTL" "$TTL"
gen_conf_row "Chunk size [kB]" "CHUNK_SIZE" "$CHSIZE"
gen_conf_row "Number of accepted chunks in parallel" "MAX_ACCEPTED_CHUNKS" "$ACCEPTED"
gen_conf_row "Timeout before check chunk [seconds]" "COMPUTATION_TIMEOUT" "$CTIME"
gen_conf_row "How many retries for each chunk before resend" "TRIES_BEFORE_RESEND" "$TRIES"
gen_conf_row "Neighbor check interval [seconds]" "NEIGHBOR_CHECK_TIMEOUT" "$NCHECK"
gen_conf_row "How many neighbors is enough?" "MAX_NEIGHBOR_COUNT" "$ENOUGH"
gen_conf_row "On which port listen" "LISTENING_PORT" "$LPORT"
gen_conf_row "On which port contact default" "SUPERPEER_PORT" "$CPORT"
gen_conf_row "Which address contact by default?" "SUPERPEER_ADDR" "$CADDR"
gen_conf_row "IP address used" "MY_IP" "$IP"
gen_conf_row "What quality of encoding use?" "QUALITY" "$QUALITY"
gen_conf_row "Transfer buffer length [bytes]" "TRANSFER_BUF_LENGTH" "1048576"

echo "Configuration successfuly created"

mkdir -p $BUILD_DIR || exit_config "Failed to create build dir."
cp "$CONFIG_FILE" "${BUILD_DIR}/CONF"
cp "run_tests.sh" "${BUILD_DIR}/run_tests.sh"
cp -r "${WORKING_DIR}/lists" "$BUILD_DIR"
arch=32
uname -a | grep "x86_64"
[[ $? == 0 ]] && arch=64
echo "Building for ${arch}-bit"
cp "Makefile_New.$arch" "Makefile"

echo "Running make..."
make || exit_config "Failed to run the make command"
mv VideoCompression "$BUILD_DIR"
echo "Cleaning..."
cleanup
echo "Successfuly installed. Please run ./VideoCompression in the bin/ directory."
