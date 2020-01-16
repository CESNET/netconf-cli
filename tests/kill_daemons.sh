set -eux -o pipefail
shopt -s failglob

RET=0
pkill -9 netopeer2 || RET=$?
pkill -9 sysrepo-plugind || RET=$?
pkill -9 sysrepod || RET=$?
exit $RET
