#!/usr/bin/env bash

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
  ${SYSREPOCTL} -C
  ${SYSREPOCTL} --uninstall "${MODULE}" -a || true
  ${SYSREPOCTL} -C
  ${SYSREPOCTL} --search-dirs "${YANG_DIR}" --install "${1}" -a
  JSON_DATA="${YANG_DIR}/${MODULE}.json"
  XML_DATA="${YANG_DIR}/${MODULE}.startup.xml"
  if [[ -f "${JSON_DATA}" ]] ;then
    ${SYSREPOCFG} -d startup -f json "${MODULE}" -i "${JSON_DATA}" -a
  elif [[ -f "${XML_DATA}" ]]; then
    ${SYSREPOCFG} -d startup -f xml "${MODULE}" -i "${XML_DATA}" -a
  fi
elif [[ "${MODE}" == "uninstall" ]]; then
  ${SYSREPOCTL} --uninstall "${MODULE}" -a
else
  echo "Mode of operation not specified"
  exit 1
fi
