![platform](https://img.shields.io/static/v1?label=platform&message=osx-64%20|%20win-32%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-xslt-v2)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-xslt-v2/total)

# 4d-plugin-xslt-v2

XSLT based on [libxml](http://xmlsoft.org/libxslt/).

### Version

<img width="32" height="32" src="https://user-images.githubusercontent.com/1725068/73986501-15964580-4981-11ea-9ac1-73c5cee50aae.png"> <img src="https://user-images.githubusercontent.com/1725068/73987971-db2ea780-4984-11ea-8ada-e25fb9c3cf4e.png" width="32" height="32" />

### Syntax

[miyako.github.io](https://miyako.github.io/2019/09/17/4d-plugin-xslt.html)

### Note

to compile typical UNIX/GNU tool for Apple Silicon

run 

```
autoreconf -if
```

set

```
export CC="gcc"
export CXX="g++"
export CCFLAGS="-arch arm64"
export CXXFLAGS="-arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
export CFLAGS="-arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
export LDFLAGS="-arch arm64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
export CPPFLAGS=""
```

also useful to set `-L` for `LDFLAGS` and `-I` for `CPPFLAGS`

then 

```
./configure --host=aarch64-apple-darwin19.6.0
```

in case of an Intel Mac


### Examples

```
$params:=New object
$params.str1:="'test'"

  //reserved: xmlParserOption,xslParserOption

$xslPath:=Get 4D folder(Current resources folder)+"sample.xsl"
$xmlPath:=Get 4D folder(Current resources folder)+"apple.svg"

DOCUMENT TO BLOB($xmlPath;$xmlData)
DOCUMENT TO BLOB($xslPath;$xslData)

CONVERT FROM TEXT($xmlPath;"utf-8";$xmlFile)
CONVERT FROM TEXT($xslPath;"utf-8";$xslFile)

$xsltData:=XSLT Apply stylesheet ($xmlFile;$xslFile;$params)
$xslt:=Convert to text($xsltData;"utf-8")

$xsltData:=XSLT Apply stylesheet ($xmlData;$xslData;$params)
$xslt:=Convert to text($xsltData;"utf-8")
```
