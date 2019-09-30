set -e
. ./iso.sh

bochs -q -rc .bochsrc_cmds
