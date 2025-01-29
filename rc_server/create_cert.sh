#!/bin/sh

OUTPUT_DIR="./"
CACFG="./ca.conf"
CAKEY="./rootCA.key"
CACERT="./rootCA.crt"
SERIAL="`cat serial.txt`"
SERIAL=$((SERIAL+1))

echo ${SERIAL}

create_config () {
  cat > ${CONFIG_FILE} << EOF
[req]
distinguished_name = req_distinguished_name
prompt = no

[req_distinguished_name]
CN=${COMMON_NAME}
O=LinuxServerSysteme
EOF
}

create_cert () {
  OUTPUT_DIR=${COMMON_NAME}

  rm -R ${OUTPUT_DIR}
  mkdir -p ${OUTPUT_DIR}

  local base=${OUTPUT_DIR}/${COMMON_NAME}

  CONFIG_FILE="${base}.conf"

  create_config

  echo "Creating certificate ${base}.key"
  openssl genrsa -aes256 -out ${base}.key 4096
  if [ $? -ne 0 ];
  then
    echo "Failed"
    exit 1
  fi


  echo "Creating signing request ${base}.csr"
  openssl req -config ${CONFIG_FILE} -sha512 -new -utf8 -key ${base}.key -out ${base}.csr

  if [ $? -ne 0 ];
  then
      echo "Failed"
      exit 1
  fi

  echo "Creating certificate ${base}.crt"
  echo "Serial: ${SERIAL}"

  cat v3.ext | sed s/%%DOMAIN%%/${COMMON_NAME}/g > /tmp/_v3.ext
  openssl x509 -req -days 365 -in ${base}.csr -CA ${CACERT} -CAkey ${CAKEY} -CAcreateserial -out ${base}.crt -extfile /tmp/_v3.ext

  if [ $? -ne 0 ];
  then
    echo "Failed"
    exit 1
  fi

  echo ${SERIAL} > serial.txt

  echo "Combining key and crt into ${base}.pem"
  cat ${base}.key > ${base}.pem
  cat ${base}.crt >> ${base}.pem

  echo "Generating Nginx Certificate Chain"

  cat ${base}.crt > ${OUTPUT_DIR}/fullchain.crt
  cat ${CACERT} >> ${OUTPUT_DIR}/fullchain.crt

  echo "Removing Password from keyfile"
  openssl rsa -in ${base}.key -out ${OUTPUT_DIR}/privkey.key

  chmod 777 -R ${OUTPUT_DIR}
}

create_ca () {
  echo "Creating CA key ${CAKEY}"
  openssl genrsa -aes256 -out ${CAKEY} 4096 > /dev/null
  if [ $? -ne 0 ];
  then
    echo "Failed"
    exit 1
  fi

  echo "Creating CA certificate ${CACERT}"
  #-config ${CACFG}
  openssl req -x509 -new -nodes -days 3999 -sha512 -utf8 -key ${CAKEY} -out ${CACERT} > /dev/null
  if [ $? -ne 0 ];
  then
    echo "Failed"
    exit 1
  fi
}

usage () {
cat << EOF
-h              show this help
-C COMMONNAME   set CommonName
-A              create_ca
-B              create_cert

Example: ./certs.sh -C hostname -B

EOF
}

mkdir -p "${OUTPUT_DIR}"

while getopts "hABC:" OPTION
do
  case ${OPTION} in
    h)
      usage
      exit 1
      ;;

    C)
      COMMON_NAME=${OPTARG}
      ;;

    A)
      create_ca
      exit
   		;;

    B)
      if [ -z ${COMMON_NAME} ]
      then
        echo "Common Name must be specified"
      else
        create_cert
      fi
      exit
      ;;

    ?)
      usage
      exit
      ;;
  esac
done
