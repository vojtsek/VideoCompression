#! /bin/bash

# transformConfig filename chukn_size buffer_size
function transformConfig {
	cat $1 | sed 's/CHUNK_SIZE \([0-9]*\)/CHUNK_SIZE '$2'/' > NEWCONF
	mv NEWCONF $1
	cat $1 | sed 's/TRANSFER_BUF_LENGTH \([0-9]*\)/TRANSFER_BUF_LENGTH '$3'/' > NEWCONF
	mv NEWCONF $1
}

function digIPv6 {
  ifconfig | grep global | sed -n 's/.*inet6 \([0-9a-f:]*\) .*$/\1/p'
}

# digIPv4 hostname
function lookupIPv4 {
  nslookup $1 | sed -n 's/Address: \([0-9.]*\)*$/\1/p'
}

# spawnClient hostname port
function spawnClient {
  #addr="::ffff:"`lookupIPv4 $1`
  command="cd ~/VideoCompression/bin && ./VideoCompression -a ~$2 -n ${contact_addr}~${contact_port} -h ${root_dir}/run${run} nostdin"
  ssh -tt "$1" "$command" &
}

# killAll pattern_to_grep
function killAll {
  cmd="ps -ef"
  for exp in $@; do
    cmd="$cmd | grep $exp"
  done
  cmd="$cmd | awk '{print \$2}' | tr '\n' ' '"
  echo "'$cmd'"
  eval "result=\$($cmd)"
  for pid in $result; do
    kill -2 $pid
    kill $pid
  done
}
# runTest run_number test_file
function runTest {
  export run=$1

  cat $test_file | while read action argument rest; do
    case $action in
    "spawn")
      hostname="$argument"
      ports=$rest
      if [[ $hostname == $(hostname) ]]; then
        continue
      fi
      for port in $ports; do
        spawnClient $hostname $port
        echo "Spawning $hostname:$port"
      done
      ;;
    "wait")
      sleep $argument
      echo "waited for $argument secs"
      ;;
    "kill")
      killAll VideoCompression ssh $argument $rest
    esac
  done
}

export test_file=$1
eval "version=\$(head -n1 $test_file)"
eval "file=\$(head -n2 $test_file | tail -n1)"
eval "runs=\$(head -n3 $test_file | tail -n1)"

cat <<END
version: $version
file: $file
number of runs: $runs
END
export contact_addr="::ffff:"$(digIPv4)
[[ $version == "v6" ]] && export contact_addr=$(digIPv6)
export contact_port="6666"
export root_dir=$(pwd)"/"$(date +test_%m_%d_%H-%M-%S)
mkdir -p $root_dir

sum_t=0
avg_t=0
eval "nodes_count=\$(grep spawn $test_file | wc -l)"

for i in `seq 1 $runs`; do
  echo "running iteration $i"
  HD=$root_dir'/run'$i
  mkdir -p $HD
  start_t=$(date +%s)
  runTest $i &
  cd ~/VideoCompression/bin && ./VideoCompression -a ${contact_addr}~${contact_port} -i $file -h "${HD}/${contact_port}" nostdin
  end_t=$(date +%s)
  exec_t=$(( $end_t - $start_t ))
  killAll VideoCompression
  sum_t=$(( sum_t + exec_t ))
  echo "took $exec_t secs"
done


avg_t=$(( sum_t / runs ))
cat <<END
time: $sum_t
average: $avg_t
number of participants: $nodes_count
END
                                                                                               105,3         Bot


