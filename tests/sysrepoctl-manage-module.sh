#!/bin/bash

set -eux -o pipefail
shopt -s failglob

SYSREPOCTL="${1}"
shift
if [[ ! -x "${SYSREPOCTL}" ]]; then
  echo "Cannot locate \$SYSREPOCTL"
  exit 1
fi

SYSREPOCFG="${1}"
shift
if [[ ! -x "${SYSREPOCFG}" ]]; then
  echo "Cannot locate \$SYSREPOCFG"
  exit 1
fi

MODE="${1}"
shift

if [[ ! -f "${1}" ]]; then
  echo "No YANG file specified"
  exit 1
fi

MODULE=$(basename --suffix .yang "${1}")
YANG_DIR=$(dirname "${1}")

if [[ "${MODE}" == "install" ]]; then
  ${SYSREPOCTL} --uninstall --module "${MODULE}" || true
  ${SYSREPOCTL} --install --yang "${1}"
  JSON_DATA="${YANG_DIR}/${MODULE}.json"
  XML_DATA="${YANG_DIR}/${MODULE}.startup.xml"
  if [[ -f "${JSON_DATA}" ]] ;then
    ${SYSREPOCFG} -d startup -f json "${MODULE}" -i "${JSON_DATA}"
  elif [[ -f "${XML_DATA}" ]]; then
    ${SYSREPOCFG} -d startup -f xml "${MODULE}" -i "${XML_DATA}"
  fi
elif [[ "${MODE}" == "uninstall" ]]; then
  ${SYSREPOCTL} --uninstall --module "${MODULE}"
else
  echo "Mode of operation not specified"
  exit 1
fi
