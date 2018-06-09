## This is Apple open source program


Link is [here](https://opensource.apple.com/tarballs/bash/)


## Compile Requirements

Xcode, Xcode command tools, ldid2 and iOSOpenDev installed.

Because Xcode do not spport to compile static execute command for iOS devices, so you need to install iOSOpenDev, for >= Xcode 8, you need some [patch](https://github.com/henrayluo/iOSOpenDevInstallFix) to iOSOpenDev before install it.

**Note**: compile ldid2 [here](https://github.com/GaryniL/ldid) for codesign.

```
ldid2 -S bash
```






## Credits

This project features work from Apple open source community, gnu, openbsd, and all other community.



