# nBlock
A network-wide web content filtering & blocking [eCAP](http://www.e-cap.org/) adapter for [Squid](http://www.squid-cache.org/).
nBlock Aims to replace all sorts of privacy enhancing browser plugins like [AdBlock Plus](https://adblockplus.org/), [Decentraleyes](https://github.com/Synzvato/decentraleyes/wiki/Simple-Introduction), [Self-Destructing Cookies](https://addons.mozilla.org/en-US/firefox/addon/self-destructing-cookies/?src=api) and fills the content filtering gaps that come with DNS only filters like [Pi-Hole](https://pi-hole.net).

Please read the [installation instructions](https://github.com/notracking/nBlock/blob/master/INSTALL.md) on how to set up your own instance of nBlock.

## Project status
In its current form nBlock should be treated as a proof of concept, though some of the base features like Adblock Plus and DNS network filtering have already been implemented.
Below you can find some of the project goals and current status. This list is not complete nor final.

**Client Request MOD**
- [x] DNSMASQ blocklist netfilter
- [x] Adblock plus based request netfilters
- [x] Rules caching
- [ ] CDN referer stripping / caching (Decentraleyes)
- [ ] Strip cookies (Self destructing cookies)
- [ ] Auto (re)routing (proxychains)

**Server Response MOD**
- [ ] Html page content analysis
- [ ] gZip decoding / compression
- [ ] iframe inspection (not possible in reqmod)
- [ ] Cosmetic filtering (CSS / XPath)

**General**
- [ ] Browser user interface (disable, add rules, remove rules, control squid settings etc)
- [ ] Block list optimizer (remove trumped rules, dupes etc)
- [ ] Landing page / user control for node-unique self signed certificates (https-bump)
