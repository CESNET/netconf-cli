import sysrepo_subscription_py as sr_sub
import netconf_cli_py as nc

c = nc.NetconfAccess(socketPath = "@NETOPEER_SOCKET_PATH@")
data = c.getItems("/ietf-netconf-monitoring:netconf-state/datastores")
for (k, v) in data:
    print(f"{k}: {type(v)} {v}", flush=True)

if len(data) == 0:
    print("ERROR: No data returned from NETCONF")
    exit(1)

subscription = sr_sub.SysrepoSubscription("example-schema")
xpath = "/example-schema:leafInt32"
for EXPECTED in (599, 59, "61"):
    c.setLeaf(xpath, EXPECTED)
    c.commitChanges()
    data = c.getItems(xpath)
    (_, value) = next(filter(lambda keyValue: keyValue[0] == xpath, data))
    if value != EXPECTED:
        if isinstance(EXPECTED, str):
            if str(value) != EXPECTED:
                print(f"ERROR: leafInt32 not updated (via string) to {EXPECTED}")
                exit(1)
        else:
            print(f"ERROR: leafInt32 not updated to {EXPECTED}")
            exit(1)
try:
    c.setLeaf(xpath, "blesmrt")
    c.commitChanges()
    print("ERROR: setting integer to a string did not error out")
    exit(1)
except RuntimeError:
    pass

exit(0)
