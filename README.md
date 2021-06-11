# TinyOWS

TinyOWS is a simple WFS-T server based on PostGIS spatial database.

## Documentation Home

https://mapserver.org/tinyows/

## Approach

TinyOWS was written with the following things in mind:
- KISS approach !
- As OGC standard compliant as possible, aiming to support:
   - WFS (1.0 and 1.1)
   - FE (1.0 and 1.1)
- Performance is important, maps are cool when they're quick to display
- Clean source code

## Code quality policy

All code must meet the following rules :
- gcc -c99 -pedantic -Wall compile without any warning (make)
- Unit test with Valgrind error and leak free (make valgrind)
- Pass throught OGC CITE WFS-T tests (1.0.0 and 1.1.0 SF-0)

Code dynamically linked with the following other librairies:
- PostgreSQL
- libxml 2.9.x
- flex

## Credits

In memory of Olivier Courtin, the original developer of TinyOWS and
visionary.


