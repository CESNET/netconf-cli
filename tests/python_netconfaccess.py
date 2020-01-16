import netconf_cli_py as nc

c = nc.NetconfAccess(socketPath = "@NETOPEER_SOCKET_PATH@")
data = c.getItems("/ietf-netconf-server:netconf-server")
for (k, v) in data.items():
    print(f"{k}: {type(v)} {v}", flush=True)

if len(data) == 0:
    print("ERROR: No data returned from NETCONF")
    exit(1)

hello_timeout_xp = "/ietf-netconf-server:netconf-server/session-options/hello-timeout"
for EXPECTED in (599, 59, "61"):
    c.setLeaf(hello_timeout_xp, EXPECTED)
    c.commitChanges()
    data = c.getItems(hello_timeout_xp)
    if (data[hello_timeout_xp] != EXPECTED):
        if isinstance(EXPECTED, str):
            if str(data[hello_timeout_xp]) != EXPECTED:
                print(f"ERROR: hello-timeout not updated (via string) to {EXPECTED}")
                exit(1)
        else:
            print(f"ERROR: hello-timeout not updated to {EXPECTED}")
            exit(1)
try:
    c.setLeaf(hello_timeout_xp, "blesmrt")
    c.commitChanges()
    print("ERROR: setting integer to a string did not error out")
    exit(1)
except RuntimeError:
    pass

exit(0)
