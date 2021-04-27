#!/bin/sh

set -e

apt-get update

DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    gcc libxml2-dev libpq-dev libfcgi-dev \
    autoconf make flex libfl-dev \
    postgresql-12 postgresql-client postgis postgresql-12-postgis-3 postgresql-12-postgis-3-scripts \
    wget ca-certificates patch valgrind

cd "${WORK_DIR}"
./autogen.sh
CFLAGS="-Werror" ./configure
make
make install

cp /etc/postgresql/12/main/pg_hba.conf /etc/postgresql/12/main/pg_hba.conf.bak
echo "local all postgres trust" |  cat - /etc/postgresql/12/main/pg_hba.conf.bak > /etc/postgresql/12/main/pg_hba.conf
echo "host all all 127.0.0.1/32 trust" |  cat - /etc/postgresql/12/main/pg_hba.conf.bak > /etc/postgresql/12/main/pg_hba.conf
/etc/init.d/postgresql start

rm -f /etc/tinyows.xml
make install-demo
cp -f demo/tinyows_no_checkschema.xml /etc/tinyows.xml
make check

# wget https://github.com/MapServer/tinyows/commit/633ca487113d032e261a4a5c8b5f3b7850580f4f.patch
# patch -p1 -R < 633ca487113d032e261a4a5c8b5f3b7850580f4f.patch

# rm -f /etc/tinyows.xml
# make install-test100

# rm -f /etc/tinyows.xml
# make install-test110
