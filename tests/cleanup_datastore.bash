set -eux

BACKEND="$1"
shift
if [[ "$BACKEND" = "netconf" ]]; then
    pkill -f "${SYSREPO_SHM_PREFIX}_netopeer2-server"
    rm "$NETOPEER_SOCKET"
fi

rm -rf "$SYSREPO_REPOSITORY_PATH"
rm -rf "/dev/shm/$SYSREPO_SHM_PREFIX"*
