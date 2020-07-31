set -eux -o pipefail
shopt -s failglob

RET=0
pkill -9 netopeer2 || RET=$?
exit $RET
