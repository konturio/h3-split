# Description
The tool takes POLYGON or MULTIPOLYGON geometry in WKT format and cuts it by antimeridian.

![split](https://user-images.githubusercontent.com/1970037/194345638-fb912c14-1971-48b2-8346-d3f9331fe3e8.png)

# Usage
```
$ split <wkt-filename>
$ echo <wkt> | split
```
(There are input examples in `/example`.)

# Installation

## Installing in `/path/to/dir/install/bin`:
```
# Create install directory
cd /path/to/dir
export WORKDIR=$(pwd)
export PREFIX="${WORKDIR}/install"
mkdir -p "$PREFIX"

# Clone h3-split repo
git clone https://github.com/mngr777/h3-split

# Clone h3 repo
git clone https://github.com/uber/h3.git --branch v4.0.1

# Build h3 library, install in `$PREFIX` using helper script
./h3-split/build-h3.sh "$PREFIX" h3

# Build h3-split, install in `$PREFIX`
cd h3-split
./autogen.sh
mkdir -p build && cd build
CFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" ../configure --prefix "$PREFIX"
make && make install

# The tool has been installed in $WORKDIR/install/bin

cd "$WORKDIR"
```
