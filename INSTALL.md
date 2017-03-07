# How to install nBlock
This installation guide will help you setting up a development build for nBlock. It includes all operations that have to be performed on a freshly installed Debian Stretch installation to get nBlock up and running.

- Download Debian 'stretch' release from: https://www.debian.org/CD/http-ftp/
- Install on a VM ([VirtualBox](https://www.virtualbox.org/wiki/Downloads)) or anywhere else
- login with your normal (not root) account and setup sudo (in case you are not familiar on how to do this..):
```
su
apt-get update
apt-get install sudo
pico /etc/sudoers
exit
```

Install required packages
```
sudo apt-get install libecap3 libecap3-dev npm node-gyp cmake pkg-config gcc autoconf automake make wget libssl-dev git
```

Since the default Debian Squid package does not come precompiled with SSL bumping capabilities we have to build it from source.
```
cd ~
SQUID_ARCHIVE=http://www.squid-cache.org/Versions/v3/3.5/squid-3.5.13.tar.gz
wget $SQUID_ARCHIVE
tar xvf squid*.tar.gz
cd $(basename squid*.tar.gz .tar.gz)
./configure \
--prefix=/usr \
--exec-prefix=/usr \
--libexecdir=/usr/lib64/squid \
--sysconfdir=/etc/squid \
--sharedstatedir=/var/lib \
--localstatedir=/var \
--libdir=/usr/lib64 \
--datadir=/usr/share/squid \
--with-logdir=/var/log/squid \
--with-pidfile=/var/run/squid.pid \
--with-default-user=squid \
--disable-dependency-tracking \
--enable-linux-netfilter \
--with-openssl \
--without-nettle \
--enable-ecap \
--enable-ssl-crtd
make
sudo make install
```

Completing Squid installation by adding the default 'squid' user, setting local file permissions and generating the ssl database for ssl_crtd
```
groupadd squid
adduser squid --disabled-password --disabled-login --ingroup squid --no-create-home -q --gecos NA
sudo chown -R squid:squid /var/log/squid /var/cache/squid
sudo chmod 750 /var/log/squid /var/cache/squid
sudo touch /etc/squid/squid.conf
sudo chown -R root:squid /etc/squid/squid.conf
sudo chmod 640 /etc/squid/squid.conf
sudo /usr/lib64/squid/ssl_crtd -c -s /var/log/squid/ssl_db
```

Building nBlock and its dependencies from source
```
cd ~
git clone https://github.com/notracking/nBlock.git
cd nBlock/src
npm install ad-block
cd ../build
cmake ..
make
sudo make install
```

Download some example blocklists
```
sudo mkdir /etc/squid/nblock/
sudo wget https://raw.githubusercontent.com/notracking/hosts-blocklists/master/hostnames.txt -O /etc/squid/nblock/hostnames.txt
sudo wget https://raw.githubusercontent.com/notracking/hosts-blocklists/master/domains.txt -O /etc/squid/nblock/domains.txt
sudo wget https://raw.githubusercontent.com/easylist/easylist/master/easylist/easylist_general_block.txt -O /etc/squid/nblock/easylist_general_block.txt
```

Generate self signed certificates for SSL bumping
```
sudo mkdir /etc/squid/ssl
cd /etc/squid/ssl
sudo chown squid:squid /etc/squid/ssl
sudo chmod 700 /etc/squid/ssl
sudo openssl req -new -newkey rsa:2048 -sha256 -days 365 -nodes -x509 -extensions v3_ca -keyout squidCA.pem  -out squidCA.pem -subj "/C=XX/ST=XX/L=squid/O=squid/CN=squid"
```

And generate a .der file that the clients should install to avoid getting SSL errors
```
sudo openssl x509 -in squidCA.pem -outform DER -out squidCA.der
```

Add the following lines to the bottom of your squid configuration file `sudo pico /etc/squid/squid.conf`
```
# To generate domain matching certs on the fly. Default settings should be adjusted for high traffic proxies
sslcrtd_program /usr/lib64/squid/ssl_crtd -s /var/log/squid/ssl_db -M 4MB
sslcrtd_children 32 startup=5 idle=1

http_port 2244 ssl-bump generate-host-certificates=on dynamic_cert_mem_cache_size=4MB cert=/etc/squid/ssl/squidCA.pem version=1 options=NO_SSLv2,NO_SSLv3,SINGLE_DH_USE

# Use acl rules to prevent certain critical sites from being ssl bumped (eg. sites for banking etc)
acl broken_sites dstdomain .somebankingsite.com
ssl_bump none broken_sites
ssl_bump client-first all

ecap_enable on
loadable_modules /usr/local/lib/nblock_ecap_adapter.so
ecap_service ecapRequest reqmod_precache uri=ecap://nBlock/ecap/services/?mode=CLIENT_REQUEST_MODE \
                cache=10000 \
                list_hostnames=/etc/squid/nblock/hostnames.txt \
                list_domains=/etc/squid/nblock/domains.txt \
                list_adblockplus=/etc/squid/nblock/easylist_general_block.txt

ecap_service ecapResponse respmod_precache uri=ecap://nBlock/ecap/services/?mode=SERVER_RESPONSE_MODE

adaptation_access ecapRequest allow all
adaptation_access ecapResponse allow all
```

Now you can start Squid with `sudo squid -N -d3`

# Client installation
Since we are basically doing a man-in-the-middle attack on the clients of our trusted network we need them to install our previously generated root certificate (/etc/squid/ssl/squidCA.der) in order to avoid breaking down https sites.

##For FireFox:
- Open 'Preferences'
- Go to the 'Advanced' section, 'Encryption' tab
- Press the 'View Certificates' button and go to the 'Authorities' tab
- Press the 'Import' button, select the .der file that was created previously and press 'OK'

##For Internet Explorer:
- Doubleclick .der file in Explorer
- Next
- Place all certificates in the following store:
  - Trusted Root Certification Authorities
- Next
- Finish
