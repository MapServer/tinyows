#!/bin/sh

set -eu

RET=0

PGUSER=postgres
DB=tinyows_demo

for i in demo/tests/input/*; do
    echo "Running $i"
    QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt
    if ! diff -u /tmp/output.txt demo/tests/expected/$(basename $i); then
        mkdir -p demo/tests/got
        mv /tmp/output.txt demo/tests/got/$(basename $i)
        echo "Result got put in demo/tests/got/$(basename $i)"
        RET=1
    fi
done

if test "$RET" -eq "0"; then
    echo "Tests OK !"
else
    echo "Tests failed !"
    exit $RET
fi

i=demo/tests/transactions/input/wfst10_france_insert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep SUCCESS || (cat /tmp/output.txt && /bin/false)
echo "select st_astext(geom) from france where id_geofla = -1234;" | su $PGUSER -c "psql -t $DB" | grep "MULTIPOLYGON(((2171610 802820,2171611 802820,2171611 802821,2171610 802820)))"
echo "delete from france where id_geofla = -1234" | su $PGUSER -c "psql $DB"


i=demo/tests/transactions/input/wfst10_world_insert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep SUCCESS || (cat /tmp/output.txt && /bin/false)
echo "select st_astext(geom) from world where name = '-1234';" | su $PGUSER -c "psql -t $DB" | grep "MULTIPOLYGON(((2 49,2 50,3 50,2 49)))"
echo "delete from world where name = '-1234'" | su $PGUSER -c "psql $DB"


i=demo/tests/transactions/input/wfst11_france_insert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep "<wfs:totalInserted>1</wfs:totalInserted>" || (cat /tmp/output.txt && /bin/false)
echo "select st_astext(geom) from france where id_geofla = -1234;" | su $PGUSER -c "psql -t $DB" | grep "MULTIPOLYGON(((2171610 802820,2171611 802820,2171611 802821,2171610 802820)))"
echo "delete from france where id_geofla = -1234" | su $PGUSER -c "psql $DB"


i=demo/tests/transactions/input/wfst11_world_insert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep "<wfs:totalInserted>1</wfs:totalInserted>" || (cat /tmp/output.txt && /bin/false)
echo "select st_astext(geom) from world where name = '-1234';" | su $PGUSER -c "psql -t $DB" | grep "MULTIPOLYGON(((2 49,2 50,3 50,2 49)))"
echo "delete from world where name = '-1234'" | su $PGUSER -c "psql $DB"


i=demo/tests/transactions/input/wfst11_world_insert_srsname_in_wfsinsert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep "<wfs:totalInserted>1</wfs:totalInserted>" || (cat /tmp/output.txt && /bin/false)
echo "select st_astext(geom) from world where name = '-1234';" | su $PGUSER -c "psql -t $DB" | grep "MULTIPOLYGON(((2 49,2 50,3 50,2 49)))"
echo "delete from world where name = '-1234'" | su $PGUSER -c "psql $DB"


i=demo/tests/transactions/input/wfst11_geometry_less_insert.xml
echo "Running $i"
QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt || (cat /tmp/output.txt && /bin/false)
cat /tmp/output.txt | grep "<wfs:totalInserted>1</wfs:totalInserted>" || (cat /tmp/output.txt && /bin/false)
echo "select textcol from geometry_less where intcol = -1234;" | su $PGUSER -c "psql -t $DB" | grep "minus 1234"
echo "delete from geometry_less where intcol = -1234" | su $PGUSER -c "psql $DB"
