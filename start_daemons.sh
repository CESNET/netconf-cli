set -eux -o pipefail
shopt -s failglob
export ASAN_OPTIONS=verify_asan_link_order=false
~/target/bin/sysrepod -d -l4
sleep 1
~/target/bin/sysrepo-plugind -d -l4
sleep 1
~/target/bin/netopeer2-server -U -d -v2
sleep 2
ps auxww
