NPROC=$(nproc)

CFLAGS="-O2 -march=native -pipe -D_FORTIFY_SOURCE=2 -flto" \
LDFLAGS="-s -Wl,-z,now -Wl,-z,relro -flto"  \
./configure                                 \
&& make -j$(( NPROC + 1 ))                  \
&& make install
