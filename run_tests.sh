#! /bin/bash

# transformConfig filename option new_val
function transformConfig {
	cat $1 | sed 's/'$2' \([0-9]*\)/'$2' '$3'/' > NEWCONF
	mv NEWCONF $1
}

function digIPv4 {
	ip addr | sed -n 's/.*inet \([0-9.]*\)\/.*$/\1/p' | grep -v 127.0.0.1
}

function digIPv6 {
	ip addr | grep global | sed -n 's/.*inet6 \([0-9a-f:]*\)\/.*$/\1/p'
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
	HD=${root_dir}/run${run}/${2}_$$
	echo $version $contact_addr
  command="mkdir -p $HD && cd $executable_location && ./VideoCompression -a ~$2 -n ${contact_addr}~${contact_port} -h ${HD} nostdin $version && rm -rf $HD"
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
export version=$(readLine $test_file 1 )
eval "file=\$(readLine $test_file 2 )"
eval "runs=\$(readLine $test_file 3 )"
eval "quality=\$(readLine $test_file 4 )"
eval "chsize=\$(readLine $test_file 5 )"
eval "bsize=\$(readLine $test_file 6 )"

cat <<END
version: $version
file: $file
number of iterations: $runs
quality: $quality
chunk size: $chsize
buffer size: $bsize
END

transformConfig CONF "CHUNK_SIZE" $chsize
transformConfig CONF "TRANSFER_BUF_LENGTH" $bsize

export contact_addr=$(digIPv4)
[[ $version == "ipv6" ]] && export contact_addr=$(digIPv6)
export contact_port="6666"
export root_dir="/tmp/hudecekv/"$(date +test_%m_%d_%H-%M-%S)
export executable_location=~/VideoCompression/bin
export report_location=~/video_tests/report
mkdir -p $root_dir

sum_t=0
avg_t=0
eval "nodes_count=\$(grep spawn $test_file | wc -l)"

for i in `seq 1 $runs`; do
  echo "running iteration $i"
  HD="${root_dir}/run${i}/${contact_port}"
  mkdir -p $HD
  start_t=$(date +%s)
  runTest $i &
	echo $contact_addr
  cd $executable_location && ./VideoCompression -q $quality -a ${contact_addr}~${contact_port} -i $file -h "${HD}" nostdin $version
  end_t=$(date +%s)
  exec_t=$(( $end_t - $start_t ))
  killAll VideoCompression
  sum_t=$(( sum_t + exec_t ))
	echo "iteration took $exec_t secs"
	logfile=$HD/logs/job_*.out
	add=$(tail -n1 $logfile | tr -d ' ')
	splitting=$(grep Splitting $logfile | cut -d' ' -f2)
	joining=$(grep Joining $logfile | cut -d' ' -f2)
	printf "%d,%d,%d,%d,%d,%d,%s,%s,%s\n" $nodes_count $exec_t $bsize $chsize $splitting $joining $add $quality $version >> $report_location
	rm -rf ${HD}/*.mkv
done


avg_t=$(( sum_t / runs ))
cat <<END
time: $sum_t
average: $avg_t
number of participants: $nodes_count
END
cp $test_file $root_dir
tar czf ${root_dir}.tar.gz $root_dir
mv ${root_dir}.tar.gz ~/video_tests
rm -rf $root_dir
