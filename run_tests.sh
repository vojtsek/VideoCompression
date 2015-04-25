#! /bin/bash

# transformConfig filename chunk_size buffer_size
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

# readLine file line_number
function readLine {
	head -n$2 $1 | tail -n1
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
eval "version=\$(readLine 1 $test_file)"
eval "file=\$(readLine 2 $test_file)"
eval "runs=\$(readLine 3 $test_file)"
eval "export quality=\$(readLine 4 $test_file)"
eval "chsize=\$(readLine 5 $test_file)"
eval "bsize=\$(readLine 6 $test_file)"

cat <<END
version: $version
file: $file
number of runs: $runs
END

transformConfig CONF $chsize $bsize

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
  cd ~/VideoCompression/bin && ./VideoCompression -q $quality -a ${contact_addr}~${contact_port} -i $file -h "${HD}/${contact_port}" nostdin >/dev/null
  end_t=$(date +%s)
  exec_t=$(( $end_t - $start_t ))
  killAll VideoCompression
  sum_t=$(( sum_t + exec_t ))
  echo "took $exec_t secs"
	printf "%d,%d,%d,%d,%s\n" $nodes_count $exec_t $bsize $chsize $quality
done


avg_t=$(( sum_t / runs ))
cat <<END
time: $sum_t
average: $avg_t
number of participants: $nodes_count
END
