set -eux -o pipefail
shopt -s failglob
export ASAN_OPTIONS=verify_asan_link_order=false
export UBSAN_OPTIONS=halt_on_error=1
~/target/bin/sysrepod -l3
sleep 1
~/target/bin/sysrepo-plugind -l3
sleep 1
~/target/bin/netopeer2-server -U -v2
sleep 5
ps auxww
