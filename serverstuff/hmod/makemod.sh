mkdir /tmp/serverstuff/
cp ../CMakeLists.txt /tmp/serverstuff
cp ../*.c /tmp/serverstuff
cp ../*h /tmp/serverstuff
cd /tmp/serverstuff
CC=/usr/bin/arm-linux-gnueabihf-gcc-8 cmake .
make
cd -
cp /tmp/serverstuff/serverstuff ./bin/
rm -fr /tmp/serverstuff
tar czvf serverstuff.hmod bin/serverstuff install etc/init.d/S100serverstuff README.md

