#! /bin/bash

function digIPv4 {
	ifconfig | sed -n 's/.*inet \([0-9.]*\) .*$/\1/p' | grep -v 127.0.0.1
}

# digIPv4 hostname
function lookupIPv4 {
	nslookup $1 | sed -n 's/Address: \([0-9.]*\)*$/\1/p'
}

# spawnClient hostname port
function spawnClient {
	addr="::ffff:"`lookupIPv4 $1`
	command="cd ~/VideoCompression/bin && ./VideoCompression -a ${addr}~$2 -n ${contact_addr}~${contact_port}"
	ssh -tt "$1" "$command" &
}

# killAll pattern_to_grep
function killAll {
	for pid in $(ps -ef | grep $1 | awk '{print $2}' | tr "\n" " "); do
		kill $pid
	done
}

export contact_addr="::ffff:"`digIPv4`
export contact_port="6666"

cat $1 | while read hostname ports; do
	if [[ $hostname == `hostname` ]]; then
		continue
	fi
	for port in $ports; do
		spawnClient $hostname $port
		echo "Spawning $hostname:$port"
	done
done

cd ~/VideoCompression/bin && ./VideoCompression -a ${contact_addr}~${contact_port} -i futu.avi nostdin

killAll VideoCompression
