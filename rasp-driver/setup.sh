# 1. Clean out broken versions
sudo apt-get remove --purge libmicrohttpd-dev libmicrohttpd12 -y

# 2. Install dependencies
sudo apt-get install build-essential autoconf automake libtool pkg-config -y

# 3. Download and extract source
cd ~
wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.66.tar.gz
tar xzf libmicrohttpd-0.9.66.tar.gz
cd libmicrohttpd-0.9.66

# 4. Configure for ARMv6
export CFLAGS="-march=armv6 -mfpu=vfp -mfloat-abi=hard"
./configure --disable-messages

# 5. Compile and install
make -j2
sudo make install
sudo ldconfig

# 6. Verify the install
ldd /usr/local/lib/libmicrohttpd.so