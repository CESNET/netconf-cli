set -eux -o pipefail
shopt -s failglob
export ASAN_OPTIONS=verify_asan_link_order=false
~/target/bin/sysrepod
sleep 1
~/target/bin/sysrepo-plugind
sleep 1
~/target/bin/netopeer2-server -U
sleep 2
ps auxww
