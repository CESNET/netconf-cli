set -eux

BACKEND="$1"
shift
if [[ "$BACKEND" = "netconf" ]]; then
    # The `-f` argument is neccessary so that pkill matches the whole command
    # line, including stuff set by `exec -a`. Otherwise it matches the name in
    # /proc/{pid}/stat and that is usually limited to 15 characters, so
    # netopeer2-server appears as netopeer2-serve
    pkill -f "${SYSREPO_SHM_PREFIX}_netopeer2-server"
    rm "$NETOPEER_SOCKET"
fi

rm -r "$SYSREPO_REPOSITORY_PATH"
rm "/dev/shm/$SYSREPO_SHM_PREFIX"*
