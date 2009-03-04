@echo off
set PG_SHARE=C:\Program Files\PostgreSQL\8.3\share\contrib
set /p PG_SHARE=[Enter path to Postgresql share directory(default=C:\Program Files\PostgreSQL\8.3\share\contrib)]:
echo PG_SHARE is set to: %PG_SHARE%
echo on

set DB=tinyows_demo

echo "Create Spatial Database: $DB"
dropdb.exe %DB%
createdb.exe %DB%
createlang.exe plpgsql %DB%
psql.exe -f %PG_SHARE%\lwpostgis.sql %DB%
psql.exe -f %PG_SHARE%\spatial_ref_sys.sql %DB%

echo "Import layer data: world" 
shp2pgsql.exe -s 4326 -I demo\world.shp world > world.sql
psql.exe -d %DB% world.sql
echo "Import layer data: france_dept" 
shp2pgsql.exe -s 27582 -I -W latin1 demo\france_dept.shp france_dept > france_dept.sql
psql.exe -d %DB% france_dept.sql
