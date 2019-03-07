~/target/bin/sysrepod
sleep 1
~/target/bin/sysrepo-plugind
mkdir -p /home/ci/target/var-run
sleep 1
~/target/bin/netopeer2-server -U
sleep 2
